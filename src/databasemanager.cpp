#include "databasemanager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

DatabaseManager::DatabaseManager(QObject* parent) : QObject(parent) {}

DatabaseManager::~DatabaseManager() {
    close();
}

bool DatabaseManager::open(const QString& path) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path);
    if (!db.open()) {
        m_lastError = db.lastError().text();
        return false;
    }
    return createTables();
}

void DatabaseManager::close() {
    QSqlDatabase::database().close();
}

bool DatabaseManager::createTables() {
    QSqlQuery q;

    bool ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS boats (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            boat_type TEXT NOT NULL,
            capacity INTEGER NOT NULL,
            steering TEXT NOT NULL,
            propulsion TEXT NOT NULL
        )
    )");
    if (!ok) { m_lastError = q.lastError().text(); return false; }

    ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS rowers (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            skill TEXT NOT NULL DEFAULT 'Beginner',
            compatibility TEXT NOT NULL DEFAULT 'Normal',
            whitelist TEXT NOT NULL DEFAULT '',
            blacklist TEXT NOT NULL DEFAULT '',
            can_steer INTEGER NOT NULL DEFAULT 0,
            is_obmann INTEGER NOT NULL DEFAULT 0,
            propulsion_ability TEXT NOT NULL DEFAULT 'Both',
            age_band INTEGER NOT NULL DEFAULT 0,
            strength INTEGER NOT NULL DEFAULT 0,
            stroke_length INTEGER NOT NULL DEFAULT 0,
            body_size INTEGER NOT NULL DEFAULT 0,
            attr_grp1 INTEGER NOT NULL DEFAULT 0,
            attr_grp2 INTEGER NOT NULL DEFAULT 0,
            attr_val1 INTEGER NOT NULL DEFAULT 0,
            attr_val2 INTEGER NOT NULL DEFAULT 0,
            boat_whitelist TEXT NOT NULL DEFAULT '',
            boat_blacklist TEXT NOT NULL DEFAULT ''
        )
    )");
    if (!ok) { m_lastError = q.lastError().text(); return false; }

    // Migrate older DBs (idempotent — errors ignored)
    {
        struct { const char* col; const char* def; } migs[] = {
            { "age_band",       "INTEGER NOT NULL DEFAULT 0" },
            { "strength",       "INTEGER NOT NULL DEFAULT 0" },
            { "stroke_length",  "INTEGER NOT NULL DEFAULT 0" },
            { "body_size",      "INTEGER NOT NULL DEFAULT 0" },
            { "attr_grp1",      "INTEGER NOT NULL DEFAULT 0" },
            { "attr_grp2",      "INTEGER NOT NULL DEFAULT 0" },
            { "attr_val1",      "INTEGER NOT NULL DEFAULT 0" },
            { "attr_val2",      "INTEGER NOT NULL DEFAULT 0" },
            { "boat_whitelist", "TEXT NOT NULL DEFAULT ''" },
            { "boat_blacklist", "TEXT NOT NULL DEFAULT ''" },
        };
        for (auto& m : migs) {
            QSqlQuery mq;
            mq.exec(QString("ALTER TABLE rowers ADD COLUMN %1 %2").arg(m.col).arg(m.def));
        }
    }

    ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS assignments (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            created_at TEXT NOT NULL,
            locked INTEGER NOT NULL DEFAULT 0,
            details TEXT DEFAULT ''
        )
    )");
    if (!ok) { m_lastError = q.lastError().text(); return false; }

    // Migrate assignments (idempotent)
    { QSqlQuery mq; mq.exec("ALTER TABLE assignments ADD COLUMN locked INTEGER NOT NULL DEFAULT 0"); }
    { QSqlQuery mq; mq.exec("ALTER TABLE assignments ADD COLUMN details TEXT DEFAULT ''"); }

    ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS expert_settings (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL DEFAULT '0'
        )
    )");
    if (!ok) { m_lastError = q.lastError().text(); return false; }

    ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS sick_rowers (
            rower_id INTEGER PRIMARY KEY,
            FOREIGN KEY(rower_id) REFERENCES rowers(id) ON DELETE CASCADE
        )
    )");
    if (!ok) { m_lastError = q.lastError().text(); return false; }

    ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS assignment_roles (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            assignment_id INTEGER NOT NULL,
            boat_id INTEGER NOT NULL,
            rower_id INTEGER NOT NULL,
            role TEXT NOT NULL,   -- 'obmann', 'steering', 'obmann_steering'
            FOREIGN KEY(assignment_id) REFERENCES assignments(id) ON DELETE CASCADE
        )
    )");
    if (!ok) { m_lastError = q.lastError().text(); return false; }

    ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS rower_distances (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            assignment_id INTEGER NOT NULL,
            rower_id INTEGER NOT NULL,
            km INTEGER NOT NULL DEFAULT 0,
            UNIQUE(assignment_id, rower_id),
            FOREIGN KEY(assignment_id) REFERENCES assignments(id) ON DELETE CASCADE
        )
    )");
    if (!ok) { m_lastError = q.lastError().text(); return false; }

    ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS assignment_entries (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            assignment_id INTEGER NOT NULL,
            boat_id INTEGER NOT NULL,
            rower_id INTEGER NOT NULL,
            FOREIGN KEY(assignment_id) REFERENCES assignments(id) ON DELETE CASCADE,
            FOREIGN KEY(boat_id) REFERENCES boats(id),
            FOREIGN KEY(rower_id) REFERENCES rowers(id)
        )
    )");
    if (!ok) { m_lastError = q.lastError().text(); return false; }

    return true;
}

// ---- Boats ----
QList<Boat> DatabaseManager::loadBoats() {
    QList<Boat> boats;
    QSqlQuery q("SELECT id, name, boat_type, capacity, steering, propulsion FROM boats ORDER BY name");
    while (q.next()) {
        Boat b;
        b.setId(q.value(0).toInt());
        b.setName(q.value(1).toString());
        b.setBoatType(Boat::boatTypeFromString(q.value(2).toString()));
        b.setCapacity(q.value(3).toInt());
        b.setSteeringType(Boat::steeringTypeFromString(q.value(4).toString()));
        b.setPropulsionType(Boat::propulsionTypeFromString(q.value(5).toString()));
        boats.append(b);
    }
    return boats;
}

bool DatabaseManager::saveBoat(Boat& boat) {
    QSqlQuery q;
    if (boat.id() == -1) {
        q.prepare("INSERT INTO boats (name, boat_type, capacity, steering, propulsion) "
                  "VALUES (?, ?, ?, ?, ?)");
        q.addBindValue(boat.name());
        q.addBindValue(Boat::boatTypeToString(boat.boatType()));
        q.addBindValue(boat.capacity());
        q.addBindValue(Boat::steeringTypeToString(boat.steeringType()));
        q.addBindValue(Boat::propulsionTypeToString(boat.propulsionType()));
        if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
        boat.setId(q.lastInsertId().toInt());
    } else {
        q.prepare("UPDATE boats SET name=?, boat_type=?, capacity=?, steering=?, propulsion=? "
                  "WHERE id=?");
        q.addBindValue(boat.name());
        q.addBindValue(Boat::boatTypeToString(boat.boatType()));
        q.addBindValue(boat.capacity());
        q.addBindValue(Boat::steeringTypeToString(boat.steeringType()));
        q.addBindValue(Boat::propulsionTypeToString(boat.propulsionType()));
        q.addBindValue(boat.id());
        if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    }
    return true;
}

bool DatabaseManager::deleteBoat(int boatId) {
    QSqlQuery q;
    q.prepare("DELETE FROM boats WHERE id=?");
    q.addBindValue(boatId);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

// ---- Rowers ----
QList<Rower> DatabaseManager::loadRowers() {
    QList<Rower> rowers;
    QSqlQuery q("SELECT id, name, skill, compatibility, whitelist, blacklist, "
                "can_steer, is_obmann, propulsion_ability, age_band, strength, "
                "stroke_length, body_size, attr_grp1, attr_grp2, attr_val1, attr_val2, "
                "boat_whitelist, boat_blacklist "
                "FROM rowers ORDER BY name");
    while (q.next()) {
        Rower r;
        r.setId(q.value(0).toInt());
        r.setName(q.value(1).toString());
        r.setSkill(Rower::skillFromString(q.value(2).toString()));
        r.setCompatibility(Rower::compatFromString(q.value(3).toString()));
        r.setWhitelist(Rower::listFromString(q.value(4).isNull() ? "" : q.value(4).toString()));
        r.setBlacklist(Rower::listFromString(q.value(5).isNull() ? "" : q.value(5).toString()));
        r.setCanSteer(q.value(6).toInt() != 0);
        r.setIsObmann(q.value(7).toInt() != 0);
        r.setPropulsionAbility(Boat::propulsionTypeFromString(
            q.value(8).isNull() ? "Both" : q.value(8).toString()));
        r.setAgeBand(q.value(9).toInt());
        r.setStrength(q.value(10).toInt());
        r.setStrokeLength(q.value(11).toInt());
        r.setBodySize(q.value(12).toInt());
        r.setAttrGrp1(q.value(13).toInt());
        r.setAttrGrp2(q.value(14).toInt());
        r.setAttrVal1(q.value(15).toInt());
        r.setAttrVal2(q.value(16).toInt());
        r.setBoatWhitelist(Rower::listFromString(q.value(17).isNull() ? "" : q.value(17).toString()));
        r.setBoatBlacklist(Rower::listFromString(q.value(18).isNull() ? "" : q.value(18).toString()));
        rowers.append(r);
    }
    return rowers;
}

bool DatabaseManager::saveRower(Rower& rower) {
    QSqlQuery q;
    const QString wl  = Rower::listToString(rower.whitelist());
    const QString bl  = Rower::listToString(rower.blacklist());
    const QString bwl = Rower::listToString(rower.boatWhitelist());
    const QString bbl = Rower::listToString(rower.boatBlacklist());
    const QString pa  = Boat::propulsionTypeToString(rower.propulsionAbility());
    const QString sk  = Rower::skillToString(rower.skill());
    const QString co  = Rower::compatToString(rower.compatibility());

    if (rower.id() == -1) {
        q.prepare("INSERT INTO rowers (name, skill, compatibility, whitelist, blacklist, "
                  "can_steer, is_obmann, propulsion_ability, age_band, strength, "
                  "stroke_length, body_size, attr_grp1, attr_grp2, attr_val1, attr_val2, "
                  "boat_whitelist, boat_blacklist) "
                  "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
        q.addBindValue(rower.name());
        q.addBindValue(sk); q.addBindValue(co);
        q.addBindValue(wl); q.addBindValue(bl);
        q.addBindValue(rower.canSteer() ? 1 : 0);
        q.addBindValue(rower.isObmann() ? 1 : 0);
        q.addBindValue(pa);
        q.addBindValue(rower.ageBand()); q.addBindValue(rower.strength());
        q.addBindValue(rower.strokeLength()); q.addBindValue(rower.bodySize());
        q.addBindValue(rower.attrGrp1()); q.addBindValue(rower.attrGrp2());
        q.addBindValue(rower.attrVal1()); q.addBindValue(rower.attrVal2());
        q.addBindValue(bwl); q.addBindValue(bbl);
        if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
        rower.setId(q.lastInsertId().toInt());
    } else {
        q.prepare("UPDATE rowers SET name=?, skill=?, compatibility=?, whitelist=?, blacklist=?, "
                  "can_steer=?, is_obmann=?, propulsion_ability=?, age_band=?, strength=?, "
                  "stroke_length=?, body_size=?, attr_grp1=?, attr_grp2=?, "
                  "attr_val1=?, attr_val2=?, boat_whitelist=?, boat_blacklist=? WHERE id=?");
        q.addBindValue(rower.name());
        q.addBindValue(sk); q.addBindValue(co);
        q.addBindValue(wl); q.addBindValue(bl);
        q.addBindValue(rower.canSteer() ? 1 : 0);
        q.addBindValue(rower.isObmann() ? 1 : 0);
        q.addBindValue(pa);
        q.addBindValue(rower.ageBand()); q.addBindValue(rower.strength());
        q.addBindValue(rower.strokeLength()); q.addBindValue(rower.bodySize());
        q.addBindValue(rower.attrGrp1()); q.addBindValue(rower.attrGrp2());
        q.addBindValue(rower.attrVal1()); q.addBindValue(rower.attrVal2());
        q.addBindValue(bwl); q.addBindValue(bbl);
        q.addBindValue(rower.id());
        if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    }
    return true;
}

bool DatabaseManager::deleteRower(int rowerId) {
    QSqlQuery q;
    q.prepare("DELETE FROM rowers WHERE id=?");
    q.addBindValue(rowerId);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

// ---- Assignments ----
QList<Assignment> DatabaseManager::loadAssignments() {
    QList<Assignment> list;
    QSqlQuery q("SELECT id, name, created_at, locked FROM assignments ORDER BY created_at DESC");
    while (q.next()) {
        Assignment a;
        a.setId(q.value(0).toInt());
        a.setName(q.value(1).toString());
        a.setCreatedAt(QDateTime::fromString(q.value(2).toString(), Qt::ISODate));
        a.setLocked(q.value(3).toInt() != 0);
        list.append(a);
    }
    return list;
}

Assignment DatabaseManager::loadAssignment(int assignmentId) {
    QSqlQuery q;
    q.prepare("SELECT id, name, created_at, details, locked FROM assignments WHERE id=?");
    q.addBindValue(assignmentId);
    q.exec();
    Assignment a;
    if (q.next()) {
        a.setId(q.value(0).toInt());
        a.setName(q.value(1).toString());
        a.setCreatedAt(QDateTime::fromString(q.value(2).toString(), Qt::ISODate));

        a.setLocked(q.value(4).toInt() != 0);
        // Parse details JSON if present
        QString detailsJson = q.value(3).toString();
        if (!detailsJson.isEmpty()) {
            QJsonObject root = QJsonDocument::fromJson(detailsJson.toUtf8()).object();

            // Groups
            QList<SavedGroup> groups;
            for (const QJsonValue& gv : root["groups"].toArray()) {
                QJsonObject go = gv.toObject();
                SavedGroup g;
                g.name   = go["name"].toString();
                g.boatId = go["boat_id"].toInt(-1);
                for (const QJsonValue& rv : go["rower_ids"].toArray())
                    g.rowerIds << rv.toInt();
                groups << g;
            }
            a.setGroups(groups);

            // Checked boats
            QList<int> cb;
            for (const QJsonValue& v : root["checked_boats"].toArray()) cb << v.toInt();
            a.setCheckedBoatIds(cb);

            // Checked rowers
            QList<int> cr;
            for (const QJsonValue& v : root["checked_rowers"].toArray()) cr << v.toInt();
            a.setCheckedRowerIds(cr);

            // Priority order
            QStringList po;
            for (const QJsonValue& v : root["priority"].toArray()) po << v.toString();
            if (!po.isEmpty()) a.setPriorityOrder(po);
        }
    }

    // Load placement entries
    QSqlQuery eq;
    eq.prepare("SELECT boat_id, rower_id FROM assignment_entries WHERE assignment_id=?");
    eq.addBindValue(assignmentId);
    eq.exec();
    while (eq.next())
        a.assignRowerToBoat(eq.value(0).toInt(), eq.value(1).toInt());

    return a;
}

bool DatabaseManager::saveAssignment(Assignment& assignment) {
    // Serialise dialog state to JSON
    QJsonObject root;

    QJsonArray groupsArr;
    for (const SavedGroup& g : assignment.groups()) {
        QJsonObject go;
        go["name"]    = g.name;
        go["boat_id"] = g.boatId;
        QJsonArray rids;
        for (int id : g.rowerIds) rids << id;
        go["rower_ids"] = rids;
        groupsArr << go;
    }
    root["groups"] = groupsArr;

    QJsonArray cbArr;
    for (int id : assignment.checkedBoatIds()) cbArr << id;
    root["checked_boats"] = cbArr;

    QJsonArray crArr;
    for (int id : assignment.checkedRowerIds()) crArr << id;
    root["checked_rowers"] = crArr;

    QJsonArray prioArr;
    for (const QString& s : assignment.priorityOrder()) prioArr << s;
    root["priority"] = prioArr;

    QString detailsJson = QString::fromUtf8(
        QJsonDocument(root).toJson(QJsonDocument::Compact));

    QSqlDatabase::database().transaction();
    QSqlQuery q;
    if (assignment.id() == -1) {
        q.prepare("INSERT INTO assignments (name, created_at, details, locked) VALUES (?,?,?,?)");
        q.addBindValue(assignment.name());
        q.addBindValue(assignment.createdAt().toString(Qt::ISODate));
        q.addBindValue(detailsJson);
        q.addBindValue(assignment.isLocked() ? 1 : 0);
        if (!q.exec()) {
            m_lastError = q.lastError().text();
            QSqlDatabase::database().rollback();
            return false;
        }
        assignment.setId(q.lastInsertId().toInt());
    } else {
        q.prepare("UPDATE assignments SET name=?, created_at=?, details=?, locked=? WHERE id=?");
        q.addBindValue(assignment.name());
        q.addBindValue(assignment.createdAt().toString(Qt::ISODate));
        q.addBindValue(detailsJson);
        q.addBindValue(assignment.isLocked() ? 1 : 0);
        q.addBindValue(assignment.id());
        if (!q.exec()) {
            m_lastError = q.lastError().text();
            QSqlDatabase::database().rollback();
            return false;
        }
        q.prepare("DELETE FROM assignment_entries WHERE assignment_id=?");
        q.addBindValue(assignment.id());
        q.exec();
    }
    for (auto it = assignment.boatRowerMap().constBegin();
         it != assignment.boatRowerMap().constEnd(); ++it) {
        for (int rowerId : it.value()) {
            q.prepare("INSERT INTO assignment_entries (assignment_id, boat_id, rower_id) VALUES (?,?,?)");
            q.addBindValue(assignment.id());
            q.addBindValue(it.key());
            q.addBindValue(rowerId);
            if (!q.exec()) {
                m_lastError = q.lastError().text();
                QSqlDatabase::database().rollback();
                return false;
            }
        }
    }
    QSqlDatabase::database().commit();
    return true;
}

bool DatabaseManager::assignmentHasEntries(int assignmentId) {
    QSqlQuery q;
    q.prepare("SELECT COUNT(*) FROM assignment_entries WHERE assignment_id=?");
    q.addBindValue(assignmentId);
    return q.exec() && q.next() && q.value(0).toInt() > 0;
}

bool DatabaseManager::deleteAssignment(int assignmentId) {
    QSqlQuery q;
    q.prepare("DELETE FROM assignments WHERE id=?");
    q.addBindValue(assignmentId);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    q.prepare("DELETE FROM assignment_entries WHERE assignment_id=?");
    q.addBindValue(assignmentId);
    q.exec();
    return true;
}

// ---- Assignment Roles ----
bool DatabaseManager::saveRole(int assignmentId, int boatId, int rowerId, const QString& role) {
    QSqlQuery q;
    q.prepare("INSERT OR REPLACE INTO assignment_roles "
              "(assignment_id, boat_id, rower_id, role) VALUES (?,?,?,?)");
    q.addBindValue(assignmentId);
    q.addBindValue(boatId);
    q.addBindValue(rowerId);
    q.addBindValue(role);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

QMap<int, QString> DatabaseManager::loadRoles(int assignmentId) {
    QMap<int, QString> result;
    QSqlQuery q;
    q.prepare("SELECT rower_id, role FROM assignment_roles WHERE assignment_id=?");
    q.addBindValue(assignmentId);
    q.exec();
    while (q.next())
        result[q.value(0).toInt()] = q.value(1).toString();
    return result;
}

// ---- Rower Distances ----
bool DatabaseManager::saveDistance(int assignmentId, int rowerId, int km) {
    QSqlQuery q;
    q.prepare("INSERT OR REPLACE INTO rower_distances (assignment_id, rower_id, km) VALUES (?,?,?)");
    q.addBindValue(assignmentId);
    q.addBindValue(rowerId);
    q.addBindValue(km);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

QMap<int, int> DatabaseManager::loadDistances(int assignmentId) {
    QMap<int, int> result;
    QSqlQuery q;
    q.prepare("SELECT rower_id, km FROM rower_distances WHERE assignment_id=?");
    q.addBindValue(assignmentId);
    q.exec();
    while (q.next())
        result[q.value(0).toInt()] = q.value(1).toInt();
    return result;
}

QMap<int, int> DatabaseManager::loadAllDistances() {
    QMap<int, int> result;
    QSqlQuery q("SELECT rower_id, SUM(km) FROM rower_distances GROUP BY rower_id");
    while (q.next())
        result[q.value(0).toInt()] = q.value(1).toInt();
    return result;
}

// ---- Statistics ----
QList<DatabaseManager::RowerStats> DatabaseManager::loadStats() {
    QList<RowerStats> stats;

    // All rowers
    QSqlQuery rq("SELECT id, name FROM rowers ORDER BY name");
    QMap<int, RowerStats> byId;
    while (rq.next()) {
        RowerStats s;
        s.rowerId = rq.value(0).toInt();
        s.name    = rq.value(1).toString();
        byId[s.rowerId] = s;
    }

    // Total role counts
    QSqlQuery roleQ("SELECT rower_id, role FROM assignment_roles");
    while (roleQ.next()) {
        int rid   = roleQ.value(0).toInt();
        QString r = roleQ.value(1).toString();
        if (!byId.contains(rid)) continue;
        if (r == "obmann" || r == "obmann_steering") byId[rid].obmannCount++;
        if (r == "steering" || r == "obmann_steering") byId[rid].steeringCount++;
    }

    // Total km
    QMap<int, int> allKm = loadAllDistances();
    for (auto it = allKm.constBegin(); it != allKm.constEnd(); ++it)
        if (byId.contains(it.key())) byId[it.key()].totalKm = it.value();

    // Recent 3 sessions per rower
    // Get the 3 most recent distinct assignment IDs
    QSqlQuery recentQ("SELECT DISTINCT assignment_id FROM assignment_roles "
                      "ORDER BY assignment_id DESC LIMIT 3");
    QList<int> recentIds;
    while (recentQ.next()) recentIds << recentQ.value(0).toInt();

    if (!recentIds.isEmpty()) {
        QString inClause = "";
        for (int id : recentIds) inClause += (inClause.isEmpty() ? "" : ",") + QString::number(id);
        QSqlQuery recRoleQ(QString("SELECT rower_id, role FROM assignment_roles "
                                   "WHERE assignment_id IN (%1)").arg(inClause));
        while (recRoleQ.next()) {
            int rid   = recRoleQ.value(0).toInt();
            QString r = recRoleQ.value(1).toString();
            if (!byId.contains(rid)) continue;
            if (r == "obmann" || r == "obmann_steering") byId[rid].recentObmann++;
            if (r == "steering" || r == "obmann_steering") byId[rid].recentSteering++;
        }
    }

    for (const RowerStats& s : byId.values()) stats << s;
    return stats;
}

// ---- Sick rowers ----
QList<int> DatabaseManager::loadSickRowerIds() {
    QList<int> result;
    QSqlQuery q("SELECT rower_id FROM sick_rowers");
    while (q.next()) result << q.value(0).toInt();
    return result;
}
bool DatabaseManager::setSickRower(int rowerId, bool sick) {
    QSqlQuery q;
    if (sick) {
        q.prepare("INSERT OR IGNORE INTO sick_rowers (rower_id) VALUES (?)");
    } else {
        q.prepare("DELETE FROM sick_rowers WHERE rower_id=?");
    }
    q.addBindValue(rowerId);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}
bool DatabaseManager::isRowerSick(int rowerId) {
    QSqlQuery q;
    q.prepare("SELECT 1 FROM sick_rowers WHERE rower_id=?");
    q.addBindValue(rowerId);
    q.exec();
    return q.next();
}

bool DatabaseManager::setAssignmentLocked(int assignmentId, bool locked) {
    QSqlQuery q;
    q.prepare("UPDATE assignments SET locked=? WHERE id=?");
    q.addBindValue(locked ? 1 : 0);
    q.addBindValue(assignmentId);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

QMap<QPair<int,int>,int> DatabaseManager::loadCoOccurrence() {
    QMap<QPair<int,int>,int> result;
    // Count how many assignments each pair of rowers shared a boat
    QSqlQuery q("SELECT ae1.rower_id, ae2.rower_id "
                "FROM assignment_entries ae1 "
                "JOIN assignment_entries ae2 "
                "  ON ae1.assignment_id = ae2.assignment_id "
                "  AND ae1.boat_id = ae2.boat_id "
                "  AND ae1.rower_id < ae2.rower_id");
    while (q.next()) {
        QPair<int,int> key(q.value(0).toInt(), q.value(1).toInt());
        result[key]++;
    }
    return result;
}

// ---- Expert Settings ----
bool DatabaseManager::saveExpertSetting(const QString& key, double value) {
    QSqlQuery q;
    q.prepare("INSERT OR REPLACE INTO expert_settings (key, value) VALUES (?, ?)");
    q.addBindValue(key);
    q.addBindValue(QString::number(value));
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

QMap<QString, double> DatabaseManager::loadExpertSettings() {
    QMap<QString, double> result;
    QSqlQuery q("SELECT key, value FROM expert_settings");
    while (q.next()) {
        QString k = q.value(0).toString();
        if (k != "_password") result[k] = q.value(1).toDouble();
    }
    return result;
}

// ---- Password ----
QString DatabaseManager::loadPassword() {
    QSqlQuery q("SELECT value FROM expert_settings WHERE key='_password'");
    if (q.next()) return q.value(0).toString();
    return "0815";  // factory default
}
bool DatabaseManager::savePassword(const QString& pw) {
    QSqlQuery q;
    q.prepare("INSERT OR REPLACE INTO expert_settings (key, value) VALUES ('_password', ?)");
    q.addBindValue(pw);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

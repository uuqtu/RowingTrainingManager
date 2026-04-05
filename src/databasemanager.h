#pragma once
#include <QObject>
#include <QList>
#include <QMap>
#include <QPair>
#include <QString>
#include "boat.h"
#include "rower.h"
#include "assignment.h"

class QSqlDatabase;

class DatabaseManager : public QObject {
    Q_OBJECT
public:
    explicit DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager();

    bool open(const QString& path = "rowing.db");
    void close();

    // Boats
    QList<Boat> loadBoats();
    bool saveBoat(Boat& boat);       // insert or update; sets boat.id on insert
    bool deleteBoat(int boatId);

    // Rowers
    QList<Rower> loadRowers();
    bool saveRower(Rower& rower);
    bool deleteRower(int rowerId);

    // Assignments
    QList<Assignment> loadAssignments();
    Assignment loadAssignment(int assignmentId);
    bool saveAssignment(Assignment& assignment);
    bool deleteAssignment(int assignmentId);
    bool assignmentHasEntries(int assignmentId);  // true if assignment_entries rows exist

    bool setAssignmentLocked(int assignmentId, bool locked);
    QMap<QPair<int,int>,int> loadCoOccurrence();

    // Expert settings persistence
    bool saveExpertSetting(const QString& key, double value);
    QMap<QString, double> loadExpertSettings();
    QString loadPassword();
    bool    savePassword(const QString& pw);

    // Sick rowers (excluded from all assignment generation)
    QList<int> loadSickRowerIds();
    bool setSickRower(int rowerId, bool sick);
    bool isRowerSick(int rowerId);

    // Assignment roles (Obmann / Steering — persisted after display)
    bool saveRole(int assignmentId, int boatId, int rowerId, const QString& role);
    // role = "obmann" | "steering" | "obmann_steering"
    // Returns map: rowerId -> role for a given assignment
    QMap<int, QString> loadRoles(int assignmentId);

    // Rower distances
    bool saveDistance(int assignmentId, int rowerId, int km);
    QMap<int, int> loadDistances(int assignmentId);  // rowerId -> km
    QMap<int, int> loadAllDistances();               // rowerId -> total km

    // Statistics
    struct RowerStats {
        int rowerId   = 0;
        QString name;
        int obmannCount   = 0;
        int steeringCount = 0;
        int totalKm       = 0;
        // Recent 3 sessions weight (0 = not overused)
        int recentObmann   = 0;
        int recentSteering = 0;
    };
    QList<RowerStats> loadStats();

    QString lastError() const { return m_lastError; }

private:
    bool createTables();
    QString m_lastError;
};

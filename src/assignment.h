#pragma once
#include <QString>
#include <QList>
#include <QMap>
#include <QDateTime>

// Persisted session group (boat pin + member IDs)
struct SavedGroup {
    QString name;
    QList<int> rowerIds;
    int boatId = -1;          // -1 = unpinned
};

class Assignment {
public:
    Assignment();
    Assignment(int id, const QString& name, const QDateTime& createdAt);

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    QString name() const { return m_name; }
    void setName(const QString& n) { m_name = n; }

    QDateTime createdAt() const { return m_createdAt; }
    void setCreatedAt(const QDateTime& dt) { m_createdAt = dt; }

    // boatId -> list of rowerIds (the actual assignment result)
    QMap<int, QList<int>> boatRowerMap() const { return m_boatRowerMap; }
    void setBoatRowerMap(const QMap<int, QList<int>>& map) { m_boatRowerMap = map; }
    void assignRowerToBoat(int boatId, int rowerId);
    void clearBoat(int boatId);

    // --- Dialog state for full round-trip restore ---

    // Groups defined in Tab 1
    QList<SavedGroup> groups() const { return m_groups; }
    void setGroups(const QList<SavedGroup>& g) { m_groups = g; }

    // Which boat IDs were checked in Tab 2 (excluding group-pinned ones)
    QList<int> checkedBoatIds() const { return m_checkedBoatIds; }
    void setCheckedBoatIds(const QList<int>& ids) { m_checkedBoatIds = ids; }

    // Which rower IDs were checked in Tab 2 (excluding group members)
    QList<int> checkedRowerIds() const { return m_checkedRowerIds; }
    void setCheckedRowerIds(const QList<int>& ids) { m_checkedRowerIds = ids; }

    // Priority order: "Skill", "Compatibility", "Propulsion match"
    QStringList priorityOrder() const { return m_priorityOrder; }
    void setPriorityOrder(const QStringList& o) { m_priorityOrder = o; }

    bool isLocked() const { return m_locked; }
    void setLocked(bool v) { m_locked = v; }

private:
    int m_id = -1;
    QString m_name;
    QDateTime m_createdAt;
    bool m_locked = false;
    QMap<int, QList<int>> m_boatRowerMap;

    QList<SavedGroup> m_groups;
    QList<int> m_checkedBoatIds;
    QList<int> m_checkedRowerIds;
    QStringList m_priorityOrder = {"Skill", "Compatibility", "Propulsion match"};
};

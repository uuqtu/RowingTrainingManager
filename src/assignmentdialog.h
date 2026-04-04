#pragma once
#include <QDialog>
#include <QList>
#include <QMap>
#include "boat.h"
#include "rower.h"
#include "assignment.h"
#include "assignmentgenerator.h"

class QListWidget;
class QListWidgetItem;
class QLineEdit;
class QPushButton;
class QLabel;
class QTextEdit;
class QTabWidget;
class QComboBox;
class QCheckBox;
class QSpinBox;
class QTableWidget;

// A session-only "must row together" group, optionally pinned to a specific boat.
struct RowingGroup {
    QString name;
    QList<int> rowerIds;
    int boatId = -1;
};

// A steering-only person for this assignment (does not occupy a seat).
struct SteeringOnlyEntry {
    int rowerId = -1;
    int boatId  = -1;   // -1 = generator decides
};

class AssignmentDialog : public QDialog {
    Q_OBJECT
public:
    explicit AssignmentDialog(
        const QList<Boat>& boats,
        const QList<Rower>& rowers,
        QWidget* parent = nullptr
    );

    // Pass equipment limits from the main Options tab
    void setEquipmentLimits(int scullOars, int sweepOars) {
        m_globalScullOars = scullOars;
        m_globalSweepOars = sweepOars;
    }

    Assignment generatedAssignment() const { return m_assignment; }
    void loadFromAssignment(const Assignment& a);

private slots:
    void onGenerate();
    void onCheck();
    void onSelectAllBoats();
    void onSelectNoneBoats();
    void onSelectAllRowers();
    void onSelectNoneRowers();
    void onMovePriorityUp();
    void onMovePriorityDown();
    void onAutoSelectBoatsToggled(bool checked);
    // Groups
    void onAddGroup();
    void onRemoveGroup();
    void onGroupSelectionChanged();
    void onAddRowerToGroup();
    void onRemoveRowerFromGroup();
    void onGroupBoatChanged(int comboIndex);
    void refreshSelectionTabStates();
    void onSelectionChanged();
    // Options
    void onAddSteeringOnly();
    void onRemoveSteeringOnly();

private:
    void setupUi();
    QWidget* buildGroupsTab();
    QWidget* buildSelectionTab();
    QWidget* buildPriorityTab();
    QWidget* buildOptionsTab();

    ScoringPriority buildPriority() const;
    QString formatPreview(const Assignment& a) const;
    QList<Rower> rowersWithGroupsApplied(const QList<Rower>& base) const;

    QList<int> claimedRowerIds() const;
    QList<int> claimedBoatIds() const;
    void refreshAvailRowerList();
    void refreshGroupBoatCombo();
    QList<Boat>  collectSelectedBoats() const;
    QList<Rower> collectSelectedRowers() const;
    QStringList  runChecks(const QList<Boat>& boats, const QList<Rower>& rowers) const;
    void updateGenerateEnabled();

    // Auto-select boats to match rower count, respecting equipment limits
    void autoSelectBoats();

    QList<Boat>             m_boats;
    QList<Rower>            m_rowers;
    QList<RowingGroup>      m_groups;
    QList<SteeringOnlyEntry> m_steeringOnly;
    Assignment              m_assignment;

    // Tab 1 — Groups
    QListWidget* m_groupList          = nullptr;
    QListWidget* m_groupMemberList    = nullptr;
    QListWidget* m_availRowerList     = nullptr;
    QComboBox*   m_groupBoatCombo     = nullptr;
    QLabel*      m_groupBoatLabel     = nullptr;
    QPushButton* m_removeGroupBtn     = nullptr;
    QPushButton* m_addToGroupBtn      = nullptr;
    QPushButton* m_removeFromGroupBtn = nullptr;

    // Tab 2 — Boats & Rowers
    QListWidget* m_boatList         = nullptr;
    QListWidget* m_rowerList        = nullptr;
    QCheckBox*   m_autoBoatsCheck   = nullptr;

    // Tab 3 — Priority
    QListWidget* m_priorityList  = nullptr;
    QCheckBox*   m_trainingCheck = nullptr;
    QCheckBox*   m_crazyCheck    = nullptr;

    // Tab 4 — Options (equipment limits now come from main Options tab)
    // (steering-only people remain here)
    int m_globalScullOars = 0;  // 0 = no limit
    int m_globalSweepOars = 0;
    QTableWidget* m_steeringOnlyTable = nullptr;
    QComboBox*   m_soRowerCombo      = nullptr;
    QComboBox*   m_soBoatCombo       = nullptr;

    // Common
    QLineEdit*   m_nameEdit    = nullptr;
    QLabel*      m_statusLabel = nullptr;
    QPushButton* m_generateBtn = nullptr;
    QPushButton* m_checkBtn    = nullptr;
    QPushButton* m_acceptBtn   = nullptr;
    QTextEdit*   m_previewEdit = nullptr;

    QTabWidget*  m_tabs = nullptr;
};

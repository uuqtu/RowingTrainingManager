#pragma once
#include <QDialog>
#include <QList>
#include <QMap>
#include <QPair>
#include "boat.h"
#include "rower.h"
#include "assignment.h"
#include "assignmentgenerator.h"

class QListWidget;
class QDoubleSpinBox;
class QProgressBar;
class QTimer;
class QListWidgetItem;
class QLineEdit;
class QPushButton;
class QLabel;
class QTextEdit;
class QTableWidget;
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
    void setCoOccurrence(const QMap<QPair<int,int>,int>& co) { m_coOccurrence = co; }

    struct ExpertParams {
        double rankWeights[5]         = {4.0,2.0,1.0,0.5,0.5};
        double whitelistBonus         = 5.0;
        double coOccurrenceFactor     = 1.5;
        double obmannBonus            = 20.0;
        double racingBeginnerPenalty  = 8.0;
        double strengthVarianceWeight = 0.3;
        double compatSpecialSpecial   = 2.0;
        double compatSpecialSelected  = 4.0;
        double strokeSmallGap1        = 3.0;
        double strokeSmallGap2        = 12.0;
        double strokeLargePerGap      = 2.5;
        double bodySmallGap1          = 1.5;
        double bodySmallGap2          = 8.0;
        double bodyLargePerGap        = 1.0;
        double grpAttrBonus           = 3.0;
        double valAttrVarianceWeight  = 0.4;
        int    fillBoatAttempts       = 600;
        int    passAttempts           = 15;
        bool   maximizeLearning       = false;
        bool   ignoreBlacklist        = false;
        bool   ignoreBoatBlacklist    = false;
        bool   ignoreBoatWhitelist    = false;
        QString logDir;               // path to Solver/ log folder (empty=disabled)
        int    racingMinSkill         = 3;  // min SkillLevel int for Racing boats (1=Novice..7=Master)
    };
    void setExpertParams(const ExpertParams& ep) { m_expertParams = ep; }

    Assignment generatedAssignment() const { return m_assignment; }
    void loadFromAssignment(const Assignment& a);

signals:
    // Emitted when user clicks "Copy current values to Expert Settings"
    void copyToExpertSettingsRequested(double w1, double w2, double w3, double w4, double w5,
                                       int fillAttempts, int passAttempts);

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
    void onCopyToExpertSettings();

private:
    void setupUi();
    QWidget* buildGroupsTab();
    QWidget* buildSelectionTab();
    QWidget* buildPriorityTab();
    QWidget* buildOptionsTab();

    ScoringPriority buildPriority() const;
    QString formatPreview(const Assignment& a) const;
    void    populatePreviewTable(const Assignment& a);
    void    populateGraphicsTab(const Assignment& a, const ScoringPriority& priority);
    QWidget* buildPreflightTab();
    void    populateScoreTab(const Assignment& a, const ScoringPriority& priority);
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
    QList<QDoubleSpinBox*> m_weightSpins;
    QCheckBox* m_ignoreBlacklistCheck   = nullptr;
    QCheckBox* m_ignoreBoatListsCheck   = nullptr;
    QComboBox* m_racingMinSkillCombo    = nullptr;
    QCheckBox* m_preserveGroupsCheck    = nullptr;  // "Preserve groups on save"
    QSpinBox*  m_fillBoatAttemptsSpin   = nullptr;  // session-override for fillBoatAttempts
    QSpinBox*  m_passAttemptsSpin       = nullptr;  // session-override for passAttempts
    QProgressBar* m_progressBar         = nullptr;  // generation progress 0-100%
    QLabel*       m_generatingIndicator  = nullptr;  // blinking dot while generating
    QTimer*       m_indicatorTimer       = nullptr;  // drives the blink
    QCheckBox*   m_trainingCheck = nullptr;
    QCheckBox*   m_crazyCheck    = nullptr;

    // Tab 4 — Options (equipment limits now come from main Options tab)
    // (steering-only people remain here)
    int m_globalScullOars = 0;  // 0 = no limit
    int m_globalSweepOars = 0;
    QMap<QPair<int,int>,int> m_coOccurrence;
    ExpertParams m_expertParams;
    QTableWidget* m_steeringOnlyTable = nullptr;
    QComboBox*   m_soRowerCombo      = nullptr;
    QComboBox*   m_soBoatCombo       = nullptr;

    // Common
    QLineEdit*   m_nameEdit    = nullptr;
    QLabel*      m_statusLabel = nullptr;
    QPushButton* m_generateBtn = nullptr;
    QPushButton* m_checkBtn    = nullptr;
    QPushButton* m_acceptBtn   = nullptr;
    QTextEdit*   m_previewEdit  = nullptr;
    QTableWidget* m_previewTable = nullptr;   // table view of generated assignment
    QWidget*      m_scoreTabWidget    = nullptr;
    QWidget*      m_graphicsTabWidget = nullptr;
    QTabWidget*   m_previewTabs       = nullptr;

    QTabWidget*  m_tabs = nullptr;
};

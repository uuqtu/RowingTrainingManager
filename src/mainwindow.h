#pragma once
#include "printerdevice.h"
#include <QMainWindow>
#include <QTimer>
#include <QDateTime>
#include <QVBoxLayout>
#include <QList>
#include "boat.h"
#include "rower.h"
#include "assignment.h"
#include "databasemanager.h"
#include "boattablemodel.h"
#include "rowertablemodel.h"

class QTabWidget;
class QTableView;
class QListWidget;
class QListWidgetItem;
class QTextEdit;
class QPushButton;
class QComboBox;
class QStyledItemDelegate;
class QTableWidget;
class QSpinBox;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    bool openDatabase(const QString& path, const QString& teamName);
    ~MainWindow();

private slots:
    // Boats tab
    void onAddBoat();
    void onDeleteBoat();
    void onBoatChanged(int row);

    // Rowers tab
    void onAddRower();
    void onDeleteRower();
    void onRowerChanged(int row);
    void onEditRowerLists();    // opens unified four-tab lists dialog

    // Assignments tab
    void onNewAssignment();
    void onDeleteAssignment();
    void onToggleLockAssignment();
    void onCopyAssignment();
    void onRenameAssignment(int assignmentId);
    void onAssignmentSelected(QListWidgetItem* item);
    void onEditAssignment(QListWidgetItem* item);
    void onCopyToClipboard();
    void onSwitchDatabase();
    void onBackupTick();
    void onPrintAssignment();
    void onPrintStats();
    void onToggleAssignmentEditMode();
    void onSaveEditedAssignment();
    void onPrintTempAssignment();
    void onAssignmentTableCellClicked(int row, int col);

    // Distance tab
    void onAssignmentDistanceSelected(int index);
    void onDistanceChanged(int row, int col);

    // Statistics tab
    void refreshStats();
    // Options tab
    void onSickModeChanged(int rowerId, bool sick);
    void refreshSickList();
    void loadExpertSettings();
    bool checkPassword(const QString& action);  // prompts and validates

private:
    void setupUi();
    QWidget* buildBoatsTab();
    QWidget* buildRowersTab();
    QWidget* buildAssignmentsTab();
    QWidget* buildDistanceTab();
    QWidget* buildDistanceDetailTab();
    QWidget* buildStatsTab();
    QWidget* buildOptionsTab();
    QWidget* buildAnalysisTab();
    void     refreshAnalysisTab();
    void     buildAnalysisGraphics(QVBoxLayout* vl);
    void     applyLanguage(const QString& code);
    void     buildAssignmentGraphics(const Assignment& a, QVBoxLayout* vl);
    void     buildRowerDevelopmentTab(QVBoxLayout* vl);
    void     buildTrainingSuggestionsTab(QVBoxLayout* vl);
    QWidget* buildExpertTab();

    void loadAll();
    void refreshAssignmentList();
    void displayAssignment(const Assignment& assignment);
    void populateAssignmentTable(const Assignment& assignment);
    QString formatAssignmentText(const Assignment& assignment);

    // Helpers
    QString rowerName(int id) const;
    QString boatDescription(int id) const;

    DatabaseManager* m_db = nullptr;
    QString m_currentDbPath;
    QString m_currentTeamName;
    QTimer*       m_backupTimer     = nullptr;
    QDateTime     m_lastBackupMtime;           // mtime of the DB at last backup
    QString       m_backupDir;                 // backup folder path
    bool          m_backupFirstRun = true;      // show status bar msg on first backup
    BoatTableModel* m_boatModel = nullptr;
    RowerTableModel* m_rowerModel = nullptr;

    QList<Boat> m_boats;
    QList<Rower> m_rowers;
    QList<Assignment> m_assignments;
    Assignment m_currentAssignment;

    // UI elements
    QTabWidget* m_tabs = nullptr;

    // Boats tab
    QTableView* m_boatTable = nullptr;

    // Rowers tab
    QTableView* m_rowerTable = nullptr;

    // Assignments tab
    QListWidget* m_assignmentList = nullptr;
    QTextEdit*    m_assignmentView       = nullptr;
    QTableWidget* m_assignmentTable      = nullptr;
    bool          m_assignmentEditMode   = false;  // edit mode for table rower swap
    QPushButton*  m_editModeBtn          = nullptr;
    QPushButton*  m_saveEditBtn          = nullptr;
    QPushButton*  m_printTempBtn         = nullptr;
    Assignment    m_editedAssignment;    // working copy in edit mode
    QTabWidget*   m_assignmentViewTabs   = nullptr;
    QWidget*      m_assignmentScoreWidget    = nullptr;
    QWidget*      m_assignmentGraphicsWidget = nullptr;
    QWidget*      m_analysisInner = nullptr;
    QComboBox*    m_languageCombo  = nullptr;
    QString       m_currentLanguage = "en";  // ISO code
    QPushButton* m_copyBtn  = nullptr;
    QPushButton* m_printBtn = nullptr;
    PrinterDevice m_printer;

    // Distance tab
    QComboBox*   m_distAssignmentCombo = nullptr;
    QTableWidget* m_distTable = nullptr;
    bool m_distUpdating = false;

    // Stats tab
    QTableWidget* m_statsTable = nullptr;
    // Distance detail tab
    QTableWidget* m_distDetailTable = nullptr;
    // Options tab
    QTableWidget* m_sickTable = nullptr;
    QList<int> m_sickRowerIds;
    QSpinBox* m_scullOarsSpinBox  = nullptr;  // equipment limits (global)
    QSpinBox* m_sweepOarsSpinBox  = nullptr;
    QString   m_password          = "0815";  // loaded from DB on startup

    // Print copies spinbox
    QSpinBox* m_printCopiesSpinBox = nullptr;

    // Expert settings — all scoring parameters, editable at runtime
    struct ExpertSettings {
        // Priority weights by rank position
        double weightRank1 = 4.0;
        double weightRank2 = 2.0;
        double weightRank3 = 1.0;
        double weightRank4 = 0.5;
        double weightRank5 = 0.5;
        // Whitelist / co-occurrence
        double whitelistBonus      = 5.0;
        double coOccurrenceFactor  = 1.5;
        // Obmann
        double obmannBonus         = 20.0;
        // Racing/Beginner
        double racingBeginnerPenalty = 8.0;
        // Strength variance
        double strengthVarianceWeight = 0.3;
        // Compatibility soft penalties
        double compatSpecialSpecial   = 2.0;
        double compatSpecialSelected  = 4.0;
        // Stroke length penalties
        double strokeSmallGap1   = 3.0;
        double strokeSmallGap2   = 12.0;
        double strokeLargePerGap = 2.5;
        // Body size penalties
        double bodySmallGap1     = 1.5;
        double bodySmallGap2     = 8.0;
        double bodyLargePerGap   = 1.0;
        // Group / value attrs
        double grpAttrBonus          = 3.0;
        double valAttrVarianceWeight = 0.4;
        // Role selection
        double obmannAgeWeight       = 0.5;
        double obmannOverusePenalty  = 3.0;
        double steerYouthWeight      = 0.3;
        double steerOverusePenalty   = 3.0;
        double steerHeavyUsePenalty  = 8.0;   // extra flat penalty when steered ≥ threshold times
        int    steerHeavyUseThreshold= 5;     // session count that triggers heavy-use penalty
        int    overuseThreshold      = 3;
        // Generator search depth
        int    fillBoatAttempts      = 600;
        int    passAttempts          = 15;
        bool   maximizeLearning      = false;
    } m_expert;
};
// (Expert settings appended below existing content — see buildExpertTab declaration)

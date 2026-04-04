#pragma once
#include "printerdevice.h"
#include <QMainWindow>
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
    void onEditWhitelist();
    void onEditBlacklist();

    // Assignments tab
    void onNewAssignment();
    void onDeleteAssignment();
    void onAssignmentSelected(QListWidgetItem* item);
    void onEditAssignment(QListWidgetItem* item);
    void onCopyToClipboard();
    void onPrintAssignment();

    // Distance tab
    void onAssignmentDistanceSelected(int index);
    void onDistanceChanged(int row, int col);

    // Statistics tab
    void refreshStats();
    // Options tab
    void onSickModeChanged(int rowerId, bool sick);
    void refreshSickList();

private:
    void setupUi();
    QWidget* buildBoatsTab();
    QWidget* buildRowersTab();
    QWidget* buildAssignmentsTab();
    QWidget* buildDistanceTab();
    QWidget* buildDistanceDetailTab();
    QWidget* buildStatsTab();
    QWidget* buildOptionsTab();

    void loadAll();
    void refreshAssignmentList();
    void displayAssignment(const Assignment& assignment);
    void populateAssignmentTable(const Assignment& assignment);
    QString formatAssignmentText(const Assignment& assignment);

    // Helpers
    QString rowerName(int id) const;
    QString boatDescription(int id) const;

    DatabaseManager* m_db = nullptr;
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
    QTextEdit*   m_assignmentView = nullptr;
    QTableWidget* m_assignmentTable = nullptr;   // table view
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

    // Print copies spinbox
    QSpinBox* m_printCopiesSpinBox = nullptr;
};

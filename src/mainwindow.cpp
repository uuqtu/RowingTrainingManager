#include "mainwindow.h"
#include "assignmentdialog.h"
#include "assignmentgenerator.h"
#include "assignmentviewdialog.h"
#include "rowerlistsdialog.h"
#include "chartwidgets.h"

#include <QTabWidget>
#include <QTableView>
#include <QListWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QApplication>
#include <QClipboard>
#include <QLabel>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QScrollArea>
#include <QStatusBar>
#include <QTimer>
#include <QRandomGenerator>
#include <QStyledItemDelegate>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QScrollArea>
#include <QFrame>
#include <functional>
#include <QFont>
#include <QPalette>
#include <QTableWidget>
#include <QLineEdit>
#include <QPainter>
#include <QSpinBox>
#include <algorithm>

// ---------------------------------------------------------------
// Custom delegate for combo-box columns in boat table
// ---------------------------------------------------------------
class ComboBoxDelegate : public QStyledItemDelegate {
public:
    ComboBoxDelegate(QStringList options, QObject* parent = nullptr)
        : QStyledItemDelegate(parent), m_options(options) {}

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&,
                          const QModelIndex&) const override {
        auto* combo = new QComboBox(parent);
        combo->addItems(m_options);
        return combo;
    }
    void setEditorData(QWidget* editor, const QModelIndex& index) const override {
        auto* combo = qobject_cast<QComboBox*>(editor);
        if (combo) combo->setCurrentText(index.data(Qt::EditRole).toString());
    }
    void setModelData(QWidget* editor, QAbstractItemModel* model,
                      const QModelIndex& index) const override {
        auto* combo = qobject_cast<QComboBox*>(editor);
        if (combo) model->setData(index, combo->currentText(), Qt::EditRole);
    }

private:
    QStringList m_options;
};

// Spin delegate for int columns
class SpinBoxDelegate : public QStyledItemDelegate {
public:
    SpinBoxDelegate(int min, int max, QObject* parent = nullptr)
        : QStyledItemDelegate(parent), m_min(min), m_max(max) {}

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&,
                          const QModelIndex&) const override {
        auto* spin = new QSpinBox(parent);
        spin->setRange(m_min, m_max);
        return spin;
    }
    void setEditorData(QWidget* editor, const QModelIndex& index) const override {
        auto* spin = qobject_cast<QSpinBox*>(editor);
        if (spin) spin->setValue(index.data(Qt::EditRole).toInt());
    }
    void setModelData(QWidget* editor, QAbstractItemModel* model,
                      const QModelIndex& index) const override {
        auto* spin = qobject_cast<QSpinBox*>(editor);
        if (spin) model->setData(index, spin->value(), Qt::EditRole);
    }

private:
    int m_min, m_max;
};

// ---------------------------------------------------------------
// MainWindow
// ---------------------------------------------------------------
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    m_db = new DatabaseManager(this);
    if (!m_db->open()) {
        QMessageBox::critical(this, "Database Error",
            "Could not open database: " + m_db->lastError());
    }

    m_boatModel = new BoatTableModel(this);
    m_rowerModel = new RowerTableModel(this);

    setupUi();
    loadAll();
    statusBar()->showMessage("Ready — rowing.db loaded.", 3000);
}

MainWindow::~MainWindow() {}

// ---------------------------------------------------------------
void MainWindow::setupUi() {
    setWindowTitle("🚣 Rowing Team Manager");
    setMinimumSize(900, 650);
    resize(1100, 750);

    // Style
    qApp->setStyle("Fusion");
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(0x1e, 0x24, 0x2f));
    palette.setColor(QPalette::WindowText, QColor(0xe8, 0xed, 0xf5));
    palette.setColor(QPalette::Base, QColor(0x16, 0x1b, 0x25));
    palette.setColor(QPalette::AlternateBase, QColor(0x26, 0x2e, 0x3e));
    palette.setColor(QPalette::Text, QColor(0xe8, 0xed, 0xf5));
    palette.setColor(QPalette::Button, QColor(0x2a, 0x35, 0x48));
    palette.setColor(QPalette::ButtonText, QColor(0xe8, 0xed, 0xf5));
    palette.setColor(QPalette::Highlight, QColor(0x1a, 0x7a, 0xc8));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::ToolTipBase, QColor(0x2a, 0x35, 0x48));
    palette.setColor(QPalette::ToolTipText, QColor(0xe8, 0xed, 0xf5));
    qApp->setPalette(palette);

    setStyleSheet(R"(
        QMainWindow { background: #1e242f; }
        QTabWidget::pane { border: 1px solid #2a3548; background: #1e242f; }
        QTabBar::tab { background: #2a3548; color: #8899aa; padding: 8px 18px;
                       border-top-left-radius: 6px; border-top-right-radius: 6px;
                       margin-right: 2px; font-weight: 600; }
        QTabBar::tab:selected { background: #1a7ac8; color: white; }
        QTabBar::tab:hover:!selected { background: #344055; color: #cdd8e8; }
        QTableView { gridline-color: #2a3548; background: #16181f; color: #e8edf5;
                     selection-background-color: #1a7ac8; border: 1px solid #2a3548;
                     border-radius: 4px; font-size: 13px; }
        QHeaderView::section { background: #242c3a; color: #8fb4d8; font-weight: 700;
                               padding: 6px 8px; border: none; border-right: 1px solid #2a3548; }
        QPushButton { background: #2a3548; color: #cdd8e8; border: 1px solid #3a4a60;
                      padding: 6px 16px; border-radius: 5px; font-weight: 600; }
        QPushButton:hover { background: #344055; border-color: #1a7ac8; color: white; }
        QPushButton:pressed { background: #1a7ac8; }
        QPushButton#primaryBtn { background: #1a7ac8; color: white; border-color: #1a7ac8; }
        QPushButton#primaryBtn:hover { background: #1e90e8; }
        QPushButton#dangerBtn { background: #8b1a1a; color: #ffaaaa; border-color: #cc2222; }
        QPushButton#dangerBtn:hover { background: #cc2222; color: white; }
        QGroupBox { color: #8fb4d8; font-weight: 700; font-size: 12px;
                    border: 1px solid #2a3548; border-radius: 6px; margin-top: 8px;
                    padding-top: 8px; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }
        QLineEdit, QSpinBox, QComboBox { background: #242c3a; color: #e8edf5;
                                         border: 1px solid #3a4a60; border-radius: 4px;
                                         padding: 4px 8px; }
        QListWidget { background: #16181f; color: #e8edf5; border: 1px solid #2a3548;
                      border-radius: 4px; }
        QListWidget::item { padding: 6px 10px; }
        QListWidget::item:selected { background: #1a7ac8; }
        QListWidget::item:hover:!selected { background: #242c3a; }
        QTextEdit { background: #0f1219; color: #c8f0c8; font-family: monospace;
                    font-size: 13px; border: 1px solid #2a3548; border-radius: 4px; }
        QStatusBar { background: #16181f; color: #5a7a9a; }
        QScrollBar:vertical { background: #16181f; width: 10px; }
        QScrollBar::handle:vertical { background: #2a3548; border-radius: 5px; }
    )");

    m_tabs = new QTabWidget(this);
    m_tabs->addTab(buildBoatsTab(),          "Boats");
    m_tabs->addTab(buildRowersTab(),         "Rowers");
    m_tabs->addTab(buildAssignmentsTab(),    "Assignments");
    m_tabs->addTab(buildDistanceTab(),       "Distance");
    m_tabs->addTab(buildDistanceDetailTab(),"Dist. Detail");
    m_tabs->addTab(buildStatsTab(),          "Statistics");
    m_tabs->addTab(buildAnalysisTab(),       "Analysis");
    m_tabs->addTab(buildOptionsTab(),        "Options");
    m_tabs->addTab(buildExpertTab(),        "Expert Settings");
    setCentralWidget(m_tabs);
}

// ---------------------------------------------------------------
QWidget* MainWindow::buildBoatsTab() {
    auto* w = new QWidget;
    auto* layout = new QVBoxLayout(w);
    layout->setSpacing(10);
    layout->setContentsMargins(12, 12, 12, 12);

    auto* infoLabel = new QLabel("Double-click any cell to edit. Changes are saved automatically.");
    infoLabel->setStyleSheet("color: #5a7a9a; font-style: italic;");
    layout->addWidget(infoLabel);

    m_boatTable = new QTableView;
    m_boatTable->setModel(m_boatModel);
    m_boatTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_boatTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_boatTable->verticalHeader()->setVisible(false);
    m_boatTable->setAlternatingRowColors(true);
    m_boatTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_boatTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);

    // Delegates
    m_boatTable->setItemDelegateForColumn(2, new ComboBoxDelegate({"Gig", "Racing"}, m_boatTable));
    m_boatTable->setItemDelegateForColumn(3, new SpinBoxDelegate(1, 5, m_boatTable));
    m_boatTable->setItemDelegateForColumn(4, new ComboBoxDelegate({"Foot-Steered", "Hand-Steered"}, m_boatTable));
    m_boatTable->setItemDelegateForColumn(5, new ComboBoxDelegate({"Scull", "Sweep"}, m_boatTable));

    layout->addWidget(m_boatTable);

    auto* btnLayout = new QHBoxLayout;
    auto* addBtn = new QPushButton("＋  Add Boat  (Ctrl+N)");
    addBtn->setObjectName("primaryBtn");
    addBtn->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));
    auto* delBtn = new QPushButton("✕  Delete Selected");
    delBtn->setObjectName("dangerBtn");
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(delBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(addBtn, &QPushButton::clicked, this, &MainWindow::onAddBoat);
    connect(delBtn, &QPushButton::clicked, this, &MainWindow::onDeleteBoat);
    connect(m_boatModel, &BoatTableModel::boatChanged, this, &MainWindow::onBoatChanged);

    return w;
}

// ---------------------------------------------------------------
QWidget* MainWindow::buildRowersTab() {
    auto* w = new QWidget;
    auto* layout = new QVBoxLayout(w);
    layout->setSpacing(10);
    layout->setContentsMargins(12, 12, 12, 12);

    auto* infoLabel = new QLabel(
        "Double-click Name/Skill/Compat/Propulsion/Attr to edit. "
        "Click Steerer/Obmann checkboxes directly in the table. "
        "Use Whitelist/Blacklist buttons to open the selection dialog.");
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #5a7a9a; font-style: italic;");
    layout->addWidget(infoLabel);

    m_rowerTable = new QTableView;
    m_rowerTable->setModel(m_rowerModel);
    m_rowerTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_rowerTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_rowerTable->verticalHeader()->setVisible(false);
    m_rowerTable->setAlternatingRowColors(true);
    m_rowerTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_rowerTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);

    // col: 0=Id,1=Name,2=Skill,3=Compat,4=FootSteer,5=Obmann,6=Prop,7=Age,
    //      8=Strength,9=StrokeLen,10=BodySize,11=GrpAttr1,12=GrpAttr2,13=ValAttr1,14=ValAttr2
    m_rowerTable->setItemDelegateForColumn(2, new ComboBoxDelegate(
        {"Student", "Beginner", "Experienced", "Professional"}, m_rowerTable));
    m_rowerTable->setItemDelegateForColumn(3, new ComboBoxDelegate(
        {"Infinite", "Normal", "Special", "Selected"}, m_rowerTable));
    m_rowerTable->setItemDelegateForColumn(6, new ComboBoxDelegate(
        {"Scull", "Sweep", "Both"}, m_rowerTable));
    m_rowerTable->setItemDelegateForColumn(7, new ComboBoxDelegate(
        Rower::ageBandOptions(), m_rowerTable));
    m_rowerTable->setItemDelegateForColumn(8,  new SpinBoxDelegate(0, 10, m_rowerTable)); // Strength
    m_rowerTable->setItemDelegateForColumn(9,  new ComboBoxDelegate(
        Rower::strokeLengthOptions(), m_rowerTable));   // Stroke Length
    m_rowerTable->setItemDelegateForColumn(10, new ComboBoxDelegate(
        Rower::bodySizeOptions(), m_rowerTable));       // Body Size
    m_rowerTable->setItemDelegateForColumn(12, new SpinBoxDelegate(0, 10, m_rowerTable)); // GrpAttr1
    m_rowerTable->setItemDelegateForColumn(13, new SpinBoxDelegate(0, 10, m_rowerTable)); // GrpAttr2
    m_rowerTable->setItemDelegateForColumn(14, new SpinBoxDelegate(0, 10, m_rowerTable)); // ValAttr1
    m_rowerTable->setItemDelegateForColumn(15, new SpinBoxDelegate(0, 10, m_rowerTable)); // ValAttr2

    layout->addWidget(m_rowerTable);

    auto* btnLayout = new QHBoxLayout;
    auto* addBtn = new QPushButton("＋  Add Rower  (Ctrl+N)");
    addBtn->setObjectName("primaryBtn");
    addBtn->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));
    auto* delBtn = new QPushButton("✕  Delete Selected");
    delBtn->setObjectName("dangerBtn");
    auto* listsBtn = new QPushButton("📋  Lists...");
    listsBtn->setToolTip("Edit Rower Whitelist, Rower Blacklist, Boat Whitelist and Boat Blacklist for the selected rower");

    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(delBtn);
    btnLayout->addSpacing(20);
    btnLayout->addWidget(listsBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(addBtn,   &QPushButton::clicked, this, &MainWindow::onAddRower);
    connect(delBtn,   &QPushButton::clicked, this, &MainWindow::onDeleteRower);
    connect(listsBtn, &QPushButton::clicked, this, &MainWindow::onEditRowerLists);
    connect(m_rowerModel, &RowerTableModel::rowerChanged, this, &MainWindow::onRowerChanged);

    return w;
}

// ---------------------------------------------------------------
QWidget* MainWindow::buildAssignmentsTab() {
    auto* w = new QWidget;
    auto* layout = new QHBoxLayout(w);
    layout->setSpacing(10);
    layout->setContentsMargins(12, 12, 12, 12);

    // Left panel: list of assignments
    auto* leftPanel = new QWidget;
    leftPanel->setMaximumWidth(280);
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setSpacing(8);

    auto* listLabel = new QLabel("Saved Assignments");
    listLabel->setStyleSheet("color: #8fb4d8; font-weight: 700; font-size: 13px;");
    leftLayout->addWidget(listLabel);

    auto* hintLabel = new QLabel("Single-click to view · Double-click to edit");
    hintLabel->setStyleSheet("color: #445566; font-size: 11px; font-style: italic;");
    leftLayout->addWidget(hintLabel);

    m_assignmentList = new QListWidget;
    leftLayout->addWidget(m_assignmentList);

    auto* leftBtnLayout = new QHBoxLayout;
    auto* newBtn = new QPushButton("🎲  New");
    newBtn->setObjectName("primaryBtn");
    auto* delBtn = new QPushButton("✕  Delete");
    delBtn->setObjectName("dangerBtn");
    auto* lockBtn = new QPushButton("🔒 Lock");
    lockBtn->setObjectName("primaryBtn");
    lockBtn->setToolTip("Lock/unlock this assignment so it cannot be edited");
    leftBtnLayout->addWidget(newBtn);
    leftBtnLayout->addWidget(delBtn);
    leftBtnLayout->addWidget(lockBtn);
    leftLayout->addLayout(leftBtnLayout);

    layout->addWidget(leftPanel);

    // Right panel: assignment detail
    auto* rightPanel = new QWidget;
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(8);

    auto* detailLabel = new QLabel("Assignment Detail");
    detailLabel->setStyleSheet("color: #8fb4d8; font-weight: 700; font-size: 13px;");
    rightLayout->addWidget(detailLabel);

    // Two-tab view: text (default), table, and scoring detail
    auto* viewTabs = new QTabWidget;
    m_assignmentViewTabs = viewTabs;

    m_assignmentView = new QTextEdit;
    m_assignmentView->setReadOnly(true);
    m_assignmentView->setPlaceholderText("Select an assignment to view details...");
    QFont monoFont("Courier New", 12);
    m_assignmentView->setFont(monoFont);
    viewTabs->addTab(m_assignmentView, "Text");

    m_assignmentTable = new QTableWidget(0, 0);
    m_assignmentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_assignmentTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_assignmentTable->horizontalHeader()->setDefaultSectionSize(120);
    m_assignmentTable->verticalHeader()->setVisible(false);
    m_assignmentTable->setStyleSheet(
        "QTableWidget { gridline-color: #2a3548; }"
        "QHeaderView::section { background:#1a2535; color:#8fb4d8; font-weight:600; padding:4px; border:1px solid #2a3548; }");
    viewTabs->addTab(m_assignmentTable, "Table");

    // Scoring tab — populated when an assignment is selected
    m_assignmentScoreWidget = new QWidget;
    viewTabs->addTab(m_assignmentScoreWidget, "Scoring");

    rightLayout->addWidget(viewTabs, 1);

    m_copyBtn = new QPushButton("Copy to Clipboard");
    m_copyBtn->setObjectName("primaryBtn");
    m_copyBtn->setEnabled(false);

    m_printBtn = new QPushButton("Print");
    m_printBtn->setObjectName("primaryBtn");
    m_printBtn->setEnabled(false);

    m_printCopiesSpinBox = new QSpinBox;
    m_printCopiesSpinBox->setRange(1, 20);
    m_printCopiesSpinBox->setValue(1);
    m_printCopiesSpinBox->setPrefix("x");
    m_printCopiesSpinBox->setToolTip("Number of copies to print (defaults to number of boats in assignment)");
    m_printCopiesSpinBox->setEnabled(false);
    m_printCopiesSpinBox->setMaximumWidth(70);

    auto* actionRow = new QHBoxLayout;
    actionRow->addStretch();
    actionRow->addWidget(m_copyBtn);
    actionRow->addWidget(m_printCopiesSpinBox);
    actionRow->addWidget(m_printBtn);
    rightLayout->addLayout(actionRow);

    layout->addWidget(rightPanel);

    connect(newBtn,  &QPushButton::clicked, this, &MainWindow::onNewAssignment);
    connect(delBtn,  &QPushButton::clicked, this, &MainWindow::onDeleteAssignment);
    connect(lockBtn, &QPushButton::clicked, this, &MainWindow::onToggleLockAssignment);
    connect(m_assignmentList, &QListWidget::itemClicked,       this, &MainWindow::onAssignmentSelected);
    connect(m_assignmentList, &QListWidget::itemDoubleClicked, this, &MainWindow::onEditAssignment);
    connect(m_copyBtn, &QPushButton::clicked, this, &MainWindow::onCopyToClipboard);
    connect(m_printBtn, &QPushButton::clicked, this, &MainWindow::onPrintAssignment);

    return w;
}

// ---------------------------------------------------------------
void MainWindow::loadAll() {
    m_boats = m_db->loadBoats();
    m_boatModel->setBoats(m_boats);

    m_rowers = m_db->loadRowers();
    m_rowerModel->setRowers(m_rowers);

    m_assignments = m_db->loadAssignments();
    m_sickRowerIds = m_db->loadSickRowerIds();
    loadExpertSettings();
    refreshAssignmentList();
    refreshStats();
}

void MainWindow::refreshAssignmentList() {
    m_assignmentList->clear();
    if (m_distAssignmentCombo) m_distAssignmentCombo->clear();

    for (const Assignment& a : m_assignments) {
        QString prefix = a.isLocked() ? "🔒 " : "";
        auto* item = new QListWidgetItem(
            prefix + QString("%1\n%2").arg(a.name()).arg(a.createdAt().toString("dd.MM.yyyy hh:mm"))
        );
        item->setData(Qt::UserRole, a.id());
        m_assignmentList->addItem(item);

        if (m_distAssignmentCombo && a.isLocked())
            m_distAssignmentCombo->addItem(
                QString("🔒 %1  (%2)").arg(a.name())
                    .arg(a.createdAt().toString("dd.MM.yyyy")),
                a.id());
    }
}

// ---------------------------------------------------------------
// Boats
// ---------------------------------------------------------------
void MainWindow::onAddBoat() {
    bool ok;
    QString name = QInputDialog::getText(this, "New Boat", "Boat name:", QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    Boat boat;
    boat.setName(name.trimmed());
    boat.setBoatType(BoatType::Gig);
    boat.setCapacity(4);
    boat.setSteeringType(SteeringType::Steered);      // default: Foot-Steered
    boat.setPropulsionType(PropulsionType::Skull);     // default: Scull

    if (m_db->saveBoat(boat)) {
        m_boatModel->addBoat(boat);
        m_boats = m_boatModel->boats();
        statusBar()->showMessage("Boat added: " + boat.name(), 2000);
    } else {
        QMessageBox::warning(this, "Error", "Could not save boat: " + m_db->lastError());
    }
}

void MainWindow::onDeleteBoat() {
    auto sel = m_boatTable->selectionModel()->selectedRows();
    if (sel.isEmpty()) return;
    int row = sel.first().row();
    Boat boat = m_boatModel->boatAt(row);
    if (QMessageBox::question(this, "Delete Boat",
        QString("Delete boat \"%1\"?").arg(boat.name())) != QMessageBox::Yes)
        return;
    if (m_db->deleteBoat(boat.id())) {
        m_boatModel->removeBoat(row);
        m_boats = m_boatModel->boats();
        statusBar()->showMessage("Boat deleted.", 2000);
    } else {
        QMessageBox::warning(this, "Error", "Could not delete: " + m_db->lastError());
    }
}

void MainWindow::onBoatChanged(int row) {
    Boat boat = m_boatModel->boatAt(row);
    if (!m_db->saveBoat(boat)) {
        QMessageBox::warning(this, "Error", "Could not save boat: " + m_db->lastError());
    }
    m_boats = m_boatModel->boats();
}

// ---------------------------------------------------------------
// Rowers
// ---------------------------------------------------------------
void MainWindow::onAddRower() {
    bool ok;
    QString name = QInputDialog::getText(this, "New Rower", "Rower name:", QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    Rower rower;
    rower.setName(name.trimmed());
    rower.setSkill(SkillLevel::Beginner);
    rower.setCompatibility(CompatibilityTier::Normal);

    if (m_db->saveRower(rower)) {
        m_rowerModel->addRower(rower);
        m_rowers = m_rowerModel->rowers();
        statusBar()->showMessage("Rower added: " + rower.name(), 2000);
    } else {
        QMessageBox::warning(this, "Error", "Could not save rower: " + m_db->lastError());
    }
}

void MainWindow::onDeleteRower() {
    auto sel = m_rowerTable->selectionModel()->selectedRows();
    if (sel.isEmpty()) return;
    int row = sel.first().row();
    Rower rower = m_rowerModel->rowerAt(row);
    if (QMessageBox::question(this, "Delete Rower",
        QString("Delete rower \"%1\"?").arg(rower.name())) != QMessageBox::Yes)
        return;
    if (m_db->deleteRower(rower.id())) {
        m_rowerModel->removeRower(row);
        m_rowers = m_rowerModel->rowers();
        statusBar()->showMessage("Rower deleted.", 2000);
    } else {
        QMessageBox::warning(this, "Error", "Could not delete: " + m_db->lastError());
    }
}

void MainWindow::onRowerChanged(int row) {
    Rower rower = m_rowerModel->rowerAt(row);
    if (!m_db->saveRower(rower)) {
        QMessageBox::warning(this, "Error", "Could not save rower: " + m_db->lastError());
    }
    m_rowers = m_rowerModel->rowers();
}

// ---------------------------------------------------------------
// Whitelist / Blacklist — full selection dialog with rower details
// ---------------------------------------------------------------
static void editList(const QString& title, const QString& description,
                     Rower& target, const QList<Rower>& allRowers,
                     bool isWhitelist, QWidget* parent)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(title);
    dlg.setMinimumSize(480, 400);

    auto* layout = new QVBoxLayout(&dlg);

    auto* descLabel = new QLabel(description);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("color: #8fb4d8; font-style: italic;");
    layout->addWidget(descLabel);

    // Search box
    auto* searchRow = new QHBoxLayout;
    auto* searchEdit = new QLineEdit;
    searchEdit->setPlaceholderText("Filter by name…");
    searchRow->addWidget(new QLabel("Filter:"));
    searchRow->addWidget(searchEdit);
    layout->addLayout(searchRow);

    QList<int> currentList = isWhitelist ? target.whitelist() : target.blacklist();

    auto* scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    auto* scrollWidget = new QWidget;
    auto* checkLayout = new QVBoxLayout(scrollWidget);
    checkLayout->setSpacing(2);

    QList<QCheckBox*> checkboxes;
    for (const Rower& r : allRowers) {
        if (r.id() == target.id()) continue;
        QString label = QString("%1   [%2 | %3 | %4]")
                            .arg(r.name(), -18)
                            .arg(Rower::skillToString(r.skill()))
                            .arg(Rower::compatToString(r.compatibility()))
                            .arg(Boat::propulsionTypeToString(r.propulsionAbility()));
        auto* cb = new QCheckBox(label);
        cb->setProperty("rowerId", r.id());
        cb->setProperty("rowerName", r.name());
        cb->setChecked(currentList.contains(r.id()));
        checkLayout->addWidget(cb);
        checkboxes.append(cb);
    }
    checkLayout->addStretch();
    scrollArea->setWidget(scrollWidget);
    layout->addWidget(scrollArea, 1);

    // Wire filter
    QObject::connect(searchEdit, &QLineEdit::textChanged, &dlg, [&checkboxes](const QString& text) {
        for (auto* cb : checkboxes)
            cb->setVisible(text.isEmpty() ||
                           cb->property("rowerName").toString().contains(text, Qt::CaseInsensitive));
    });

    // Select all / none shortcuts
    auto* selRow = new QHBoxLayout;
    auto* selAll  = new QPushButton("Select all");
    auto* selNone = new QPushButton("Select none");
    selRow->addWidget(selAll);
    selRow->addWidget(selNone);
    selRow->addStretch();
    layout->addLayout(selRow);
    QObject::connect(selAll,  &QPushButton::clicked, &dlg, [&checkboxes]() {
        for (auto* cb : checkboxes) if (cb->isVisible()) cb->setChecked(true);
    });
    QObject::connect(selNone, &QPushButton::clicked, &dlg, [&checkboxes]() {
        for (auto* cb : checkboxes) if (cb->isVisible()) cb->setChecked(false);
    });

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        QList<int> newList;
        for (auto* cb : checkboxes)
            if (cb->isChecked())
                newList.append(cb->property("rowerId").toInt());
        if (isWhitelist) target.setWhitelist(newList);
        else             target.setBlacklist(newList);
    }
}

void MainWindow::onEditRowerLists() {
    auto sel = m_rowerTable->selectionModel()->selectedRows();
    if (sel.isEmpty()) { statusBar()->showMessage("Select a rower first.", 2000); return; }
    int row = sel.first().row();
    Rower rower = m_rowerModel->rowerAt(row);

    RowerListsDialog dlg(rower, m_rowers, m_boats, this);
    if (dlg.exec() == QDialog::Accepted) {
        Rower updated = dlg.result();
        m_rowerModel->updateRower(row, updated);
        m_db->saveRower(updated);
        m_rowers = m_rowerModel->rowers();
        statusBar()->showMessage("Lists saved for " + updated.name(), 2000);
    }
}

// ---------------------------------------------------------------
// Assignments
// ---------------------------------------------------------------
void MainWindow::onNewAssignment() {
    if (m_boats.isEmpty()) {
        QMessageBox::information(this, "No Boats", "Please add boats first.");
        return;
    }
    if (m_rowers.isEmpty()) {
        QMessageBox::information(this, "No Rowers", "Please add rowers first.");
        return;
    }

    // Exclude sick rowers from assignment generator
    QList<Rower> healthyRowers;
    for (const Rower& r : m_rowers)
        if (!m_sickRowerIds.contains(r.id())) healthyRowers << r;
    AssignmentDialog dlg(m_boats, healthyRowers, this);
    dlg.setEquipmentLimits(
        m_scullOarsSpinBox ? m_scullOarsSpinBox->value() : 0,
        m_sweepOarsSpinBox ? m_sweepOarsSpinBox->value() : 0);
    dlg.setCoOccurrence(m_db->loadCoOccurrence());
    {
        AssignmentDialog::ExpertParams ep;
        ep.rankWeights[0]=m_expert.weightRank1; ep.rankWeights[1]=m_expert.weightRank2;
        ep.rankWeights[2]=m_expert.weightRank3; ep.rankWeights[3]=m_expert.weightRank4;
        ep.rankWeights[4]=m_expert.weightRank5;
        ep.whitelistBonus=m_expert.whitelistBonus; ep.coOccurrenceFactor=m_expert.coOccurrenceFactor;
        ep.obmannBonus=m_expert.obmannBonus; ep.racingBeginnerPenalty=m_expert.racingBeginnerPenalty;
        ep.strengthVarianceWeight=m_expert.strengthVarianceWeight;
        ep.compatSpecialSpecial=m_expert.compatSpecialSpecial; ep.compatSpecialSelected=m_expert.compatSpecialSelected;
        ep.strokeSmallGap1=m_expert.strokeSmallGap1; ep.strokeSmallGap2=m_expert.strokeSmallGap2;
        ep.strokeLargePerGap=m_expert.strokeLargePerGap;
        ep.bodySmallGap1=m_expert.bodySmallGap1; ep.bodySmallGap2=m_expert.bodySmallGap2;
        ep.bodyLargePerGap=m_expert.bodyLargePerGap;
        ep.grpAttrBonus=m_expert.grpAttrBonus; ep.valAttrVarianceWeight=m_expert.valAttrVarianceWeight;
        ep.fillBoatAttempts=m_expert.fillBoatAttempts; ep.passAttempts=m_expert.passAttempts;
        dlg.setExpertParams(ep);
    }
    if (dlg.exec() == QDialog::Accepted) {
        Assignment a = dlg.generatedAssignment();
        if (m_db->saveAssignment(a)) {
            m_assignments.prepend(a);
            refreshAssignmentList();
            // Select and show the new assignment
            m_assignmentList->setCurrentRow(0);
            displayAssignment(a);
            statusBar()->showMessage("Assignment saved: " + a.name(), 3000);
        } else {
            QMessageBox::warning(this, "Error", "Could not save assignment: " + m_db->lastError());
        }
    }
}

void MainWindow::onDeleteAssignment() {
    auto* item = m_assignmentList->currentItem();
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    if (QMessageBox::question(this, "Delete Assignment",
        "Delete this assignment?") != QMessageBox::Yes) return;
    if (m_db->deleteAssignment(id)) {
        m_assignments.removeIf([id](const Assignment& a) { return a.id() == id; });
        refreshAssignmentList();
        m_assignmentView->clear();
        m_copyBtn->setEnabled(false);
        statusBar()->showMessage("Assignment deleted.", 2000);
    }
}

void MainWindow::onAssignmentSelected(QListWidgetItem* item) {
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    Assignment a = m_db->loadAssignment(id);
    m_currentAssignment = a;
    displayAssignment(a);
    m_copyBtn->setEnabled(true);
    m_printBtn->setEnabled(true);
    if (m_printCopiesSpinBox) {
        int boatCount = qMax(1, a.boatRowerMap().size());
        m_printCopiesSpinBox->setValue(boatCount);
        m_printCopiesSpinBox->setEnabled(true);
    }
}

void MainWindow::onEditAssignment(QListWidgetItem* item) {
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    Assignment existing = m_db->loadAssignment(id);
    if (existing.isLocked()) {
        // Locked: open read-only view dialog instead
        QMap<int,QString> savedRoles = m_db->loadRoles(existing.id());
        AssignmentViewDialog viewDlg(existing, m_boats, m_rowers, savedRoles, this);
        viewDlg.exec();
        return;
    }

    QList<Rower> healthyRowers2;
    for (const Rower& r : m_rowers)
        if (!m_sickRowerIds.contains(r.id())) healthyRowers2 << r;
    AssignmentDialog dlg(m_boats, healthyRowers2, this);
    dlg.setEquipmentLimits(
        m_scullOarsSpinBox ? m_scullOarsSpinBox->value() : 0,
        m_sweepOarsSpinBox ? m_sweepOarsSpinBox->value() : 0);
    dlg.setCoOccurrence(m_db->loadCoOccurrence());
    {
        AssignmentDialog::ExpertParams ep;
        ep.rankWeights[0]=m_expert.weightRank1; ep.rankWeights[1]=m_expert.weightRank2;
        ep.rankWeights[2]=m_expert.weightRank3; ep.rankWeights[3]=m_expert.weightRank4;
        ep.rankWeights[4]=m_expert.weightRank5;
        ep.whitelistBonus=m_expert.whitelistBonus; ep.coOccurrenceFactor=m_expert.coOccurrenceFactor;
        ep.obmannBonus=m_expert.obmannBonus; ep.racingBeginnerPenalty=m_expert.racingBeginnerPenalty;
        ep.strengthVarianceWeight=m_expert.strengthVarianceWeight;
        ep.compatSpecialSpecial=m_expert.compatSpecialSpecial; ep.compatSpecialSelected=m_expert.compatSpecialSelected;
        ep.strokeSmallGap1=m_expert.strokeSmallGap1; ep.strokeSmallGap2=m_expert.strokeSmallGap2;
        ep.strokeLargePerGap=m_expert.strokeLargePerGap;
        ep.bodySmallGap1=m_expert.bodySmallGap1; ep.bodySmallGap2=m_expert.bodySmallGap2;
        ep.bodyLargePerGap=m_expert.bodyLargePerGap;
        ep.grpAttrBonus=m_expert.grpAttrBonus; ep.valAttrVarianceWeight=m_expert.valAttrVarianceWeight;
        ep.fillBoatAttempts=m_expert.fillBoatAttempts; ep.passAttempts=m_expert.passAttempts;
        dlg.setExpertParams(ep);
    }
    dlg.loadFromAssignment(existing);

    if (dlg.exec() == QDialog::Accepted) {
        Assignment updated = dlg.generatedAssignment();
        // Overwrite the same DB record — keep original id and created_at
        updated.setId(existing.id());
        updated.setCreatedAt(existing.createdAt());

        if (m_db->saveAssignment(updated)) {
            for (Assignment& a : m_assignments)
                if (a.id() == updated.id()) { a = updated; break; }
            refreshAssignmentList();
            for (int i = 0; i < m_assignmentList->count(); ++i) {
                if (m_assignmentList->item(i)->data(Qt::UserRole).toInt() == updated.id()) {
                    m_assignmentList->setCurrentRow(i);
                    break;
                }
            }
            m_currentAssignment = updated;
            displayAssignment(updated);
            m_copyBtn->setEnabled(true);
            statusBar()->showMessage("Assignment updated: " + updated.name(), 3000);
        } else {
            QMessageBox::warning(this, "Error", "Could not save assignment: " + m_db->lastError());
        }
    }
}

// ---------------------------------------------------------------
QString MainWindow::rowerName(int id) const {
    for (const Rower& r : m_rowers)
        if (r.id() == id) return r.name();
    return QString("Rower#%1").arg(id);
}

QString MainWindow::boatDescription(int id) const {
    for (const Boat& b : m_boats)
        if (b.id() == id) return b.name();
    return QString("Boat#%1").arg(id);
}

QString MainWindow::formatAssignmentText(const Assignment& assignment) {
    const int W = 40;
    QString text;
    text += QString("Assignment: %1").arg(assignment.name()).left(W) + "\n";
    text += QString("Created:    %1\n").arg(assignment.createdAt().toString("dd.MM.yyyy hh:mm:ss"));
    text += QString("=").repeated(W) + "\n\n";

    // Load already-persisted roles — never re-pick on display
    QMap<int,QString> savedRoles = m_db->loadRoles(assignment.id());
    // savedRoles: rowerId -> "obmann" | "steering" | "obmann_steering"

    // If no roles have been saved yet (first display after generation), pick and persist once
    if (savedRoles.isEmpty()) {
        QList<DatabaseManager::RowerStats> stats = m_db->loadStats();
        QMap<int,int> recentObmann, recentSteering;
        for (const DatabaseManager::RowerStats& s : stats) {
            recentObmann[s.rowerId]   = s.recentObmann;
            recentSteering[s.rowerId] = s.recentSteering;
        }
        auto obmannScore = [&](int rid) -> double {
            for (const Rower& r : m_rowers) {
                if (r.id() != rid) continue;
                if (!r.isObmann()) return -1e9;
                double score = r.ageBand() * 0.5 - recentObmann.value(rid, 0) * 3.0;
                return score;
            }
            return -1e9;
        };
        auto steerScore = [&](int rid) -> double {
            for (const Rower& r : m_rowers) {
                if (r.id() != rid) continue;
                if (!r.canSteer()) return -1e9;
                int band = r.ageBand() > 0 ? r.ageBand() : 50;
                return (100 - band) * 0.3 - recentSteering.value(rid, 0) * 3.0;
            }
            return -1e9;
        };

        const auto& map = assignment.boatRowerMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            int boatId = it.key();
            Boat foundBoat;
            for (const Boat& b : m_boats) if (b.id() == boatId) { foundBoat = b; break; }
            bool isSteered  = foundBoat.id() != -1 && foundBoat.steeringType() == SteeringType::Steered;
            bool needsRoles = foundBoat.id() == -1 || foundBoat.capacity() > 2;
            if (!needsRoles) continue;

            const QList<int>& rowerIds = it.value();
            int chosenObmann = -1;
            double bestOb = -1e18;
            for (int rid : rowerIds) {
                double s = obmannScore(rid);
                if (s > -1e8 && s > bestOb) { bestOb = s; chosenObmann = rid; }
            }
            if (chosenObmann == -1 && !rowerIds.isEmpty())
                chosenObmann = rowerIds.at(QRandomGenerator::global()->bounded(
                    static_cast<quint32>(rowerIds.size())));

            int chosenSteerer = -1;
            if (isSteered) {
                double bestSt = -1e18;
                for (int rid : rowerIds) {
                    double s = steerScore(rid);
                    if (s > -1e8 && s > bestSt) { bestSt = s; chosenSteerer = rid; }
                }
            }

            // Persist once
            if (chosenObmann != -1 && chosenObmann == chosenSteerer) {
                m_db->saveRole(assignment.id(), boatId, chosenObmann, "obmann_steering");
                savedRoles[chosenObmann] = "obmann_steering";
            } else {
                if (chosenObmann != -1) {
                    m_db->saveRole(assignment.id(), boatId, chosenObmann, "obmann");
                    savedRoles[chosenObmann] = "obmann";
                }
                if (chosenSteerer != -1) {
                    m_db->saveRole(assignment.id(), boatId, chosenSteerer, "steering");
                    savedRoles[chosenSteerer] = "steering";
                }
            }
        }
    }

    // Now render using the stable savedRoles map — no further randomness
    const auto& map = assignment.boatRowerMap();
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        int boatId = it.key();
        const QList<int>& rowerIds = it.value();

        Boat foundBoat;
        for (const Boat& b : m_boats) if (b.id() == boatId) { foundBoat = b; break; }

        text += QString("= %1 =").arg(boatDescription(boatId)).left(W) + "\n";
        if (foundBoat.id() != -1)
            text += QString("  [%1|Cap:%2|%3|%4]\n")
                        .arg(Boat::boatTypeToString(foundBoat.boatType()).left(5))
                        .arg(foundBoat.capacity())
                        .arg(Boat::steeringTypeToString(foundBoat.steeringType()).left(10))
                        .arg(Boat::propulsionTypeToString(foundBoat.propulsionType()).left(6));

        bool needsRoles = foundBoat.id() == -1 || foundBoat.capacity() > 2;

        // Find Obmann and Steerer for this boat from saved roles
        int chosenObmann  = -1;
        int chosenSteerer = -1;
        if (needsRoles) {
            for (int rid : rowerIds) {
                QString role = savedRoles.value(rid);
                if (role == "obmann" || role == "obmann_steering") chosenObmann  = rid;
                if (role == "steering" || role == "obmann_steering") chosenSteerer = rid;
            }
        }

        // Print Obmann first
        if (needsRoles && chosenObmann == -1 && !rowerIds.isEmpty()) {
            text += "  *** No Obmann available for this boat! ***\n";
            text += "  *** First rower is Obmann ***\n";
        }

        if (chosenObmann != -1) {
            for (const Rower& r : m_rowers) {
                if (r.id() != chosenObmann) continue;
                QStringList tags;
                tags << "[Obmann]";
                if (chosenObmann == chosenSteerer) tags << "[Steering]";
                text += QString("  %1 %2\n").arg(r.name().left(20), -20).arg(tags.join(" "));
                break;
            }
        }
        for (int rid : rowerIds) {
            if (rid == chosenObmann) continue;
            // Look up name — search all m_rowers (including the full list)
            QString name;
            for (const Rower& r : m_rowers) if (r.id() == rid) { name = r.name(); break; }
            if (name.isEmpty()) name = QString("Rower#%1").arg(rid);
            QStringList tags;
            if (rid == chosenSteerer) tags << "[Steering]";
            text += QString("  %1%2\n")
                        .arg(name.left(22), -22)
                        .arg(tags.isEmpty() ? QString() : " " + tags.join(" "));
        }
        text += "\n";
    }
    return text;
}
void MainWindow::displayAssignment(const Assignment& assignment) {
    m_assignmentView->setPlainText(formatAssignmentText(assignment));
    if (m_assignmentTable) populateAssignmentTable(assignment);

    // Populate scoring detail tab
    if (m_assignmentScoreWidget && m_assignmentViewTabs) {
        // Replace old scoring widget with fresh one
        int scoreTabIdx = m_assignmentViewTabs->indexOf(m_assignmentScoreWidget);
        if (scoreTabIdx >= 0) {
            m_assignmentViewTabs->removeTab(scoreTabIdx);
            delete m_assignmentScoreWidget;
        }

        // Build scoring view inline (same logic as AssignmentViewDialog::buildScoreView)
        auto* outer = new QWidget;
        auto* outerVL = new QVBoxLayout(outer);
        outerVL->setContentsMargins(0, 0, 0, 0);
        auto* scroll = new QScrollArea;
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        auto* inner = new QWidget;
        auto* vl = new QVBoxLayout(inner);
        vl->setContentsMargins(8, 8, 8, 8);
        vl->setSpacing(12);

        ScoringPriority priority;
        AssignmentGenerator gen;
        QList<ScoreDetail> details = gen.computeScoreDetails(assignment, m_boats, m_rowers, priority);

        if (details.isEmpty()) {
            vl->addWidget(new QLabel("<i style='color:#556677;'>No scoring data available.</i>"));
        }

        for (const ScoreDetail& d : details) {
            QString boatName = QString("Boat#%1").arg(d.boatId);
            for (const Boat& b : m_boats) if (b.id() == d.boatId) { boatName = b.name(); break; }

            auto* boatHeader = new QLabel(
                QString("<b style='color:#8fb4d8; font-size:13px;'>%1</b>"
                        "  <span style='color:#5a7a9a;'>— Total Score: <b style='color:%2;'>%3</b></span>")
                    .arg(boatName.toHtmlEscaped())
                    .arg(d.totalScore >= 0 ? "#66cc66" : "#cc6666")
                    .arg(QString::number(d.totalScore, 'f', 2)));
            boatHeader->setStyleSheet("background:#0d1a2a; padding:4px 8px; border-radius:4px;");
            vl->addWidget(boatHeader);

            // Boat-level table
            auto* bt = new QTableWidget(0, 2);
            bt->horizontalHeader()->setVisible(false);
            bt->verticalHeader()->setVisible(false);
            bt->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
            bt->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
            bt->setEditTriggers(QAbstractItemView::NoEditTriggers);
            bt->setSelectionMode(QAbstractItemView::NoSelection);
            bt->setStyleSheet("QTableWidget{background:#0a1520;gridline-color:#1a2538;}"
                              "QTableWidget::item{padding:3px 6px;}");

            auto addBR = [&](const QString& n, const QString& v, const QString& c=""){
                int r = bt->rowCount(); bt->insertRow(r);
                auto* ni = new QTableWidgetItem(n); ni->setForeground(QColor("#8090a0"));
                auto* vi = new QTableWidgetItem(v);
                if (!c.isEmpty()) vi->setForeground(QColor(c));
                bt->setItem(r,0,ni); bt->setItem(r,1,vi);
            };
            auto fmt = [](double v){ return QString("%1%2").arg(v>=0?"+":"").arg(v,0,'f',2); };

            addBR("Mode", d.trainingMode?"Training":d.crazyMode?"Crazy":"Normal");
            addBR("Weights (wSkill/wCompat/wProp)",
                  QString("%1/%2/%3").arg(d.wSkill,0,'f',1).arg(d.wCompat,0,'f',1).arg(d.wProp,0,'f',1));
            if (!d.trainingMode) {
                addBR("  avgSkill",     QString::number(d.avgSkill,'f',2));
                addBR("  skillBalance", fmt(d.skillBalance), d.skillBalance>=0?"#66cc66":"#cc8844");
                addBR("  wSkill×(avgSkill+skillBalance)", fmt(d.wSkill*(d.avgSkill+d.skillBalance)));
                addBR("  compatPenalty (raw)", QString::number(d.compatPenalty,'f',2));
                addBR("  wCompat×(−compat)", fmt(-d.wCompat*d.compatPenalty),
                      d.compatPenalty>0?"#cc8844":"#66cc66");
            }
            addBR("  avgProp",              QString::number(d.avgProp,'f',3));
            addBR("  wProp×avgProp×3",      fmt(d.wProp*d.avgProp*3.0), "#66cc66");
            addBR("  strengthVariance",     QString::number(d.strengthVariance,'f',2));
            addBR("  −strengthVar×weight",  fmt(-d.strengthVariance*priority.strengthVarianceWeight),
                  d.strengthVariance>0?"#cc8844":"#8090a0");
            addBR("  obmannBonus",     fmt(d.obmannBonus),       d.obmannBonus>0?"#66cc66":"#8090a0");
            addBR("  racingBegPenalty",fmt(-d.racingBegPenalty), d.racingBegPenalty>0?"#cc6666":"#8090a0");
            addBR("  strokePenalty",   fmt(-d.strokePenalty),    d.strokePenalty>0?"#cc8844":"#8090a0");
            addBR("  bodyPenalty",     fmt(-d.bodyPenalty),      d.bodyPenalty>0?"#cc8844":"#8090a0");
            addBR("  grpBonus",        fmt(d.grpBonus),          d.grpBonus>0?"#66cc66":"#8090a0");
            addBR("  valPenalty",      fmt(-d.valPenalty),       d.valPenalty>0?"#cc8844":"#8090a0");
            addBR("  coOccurPenalty",  fmt(-d.coOccurrencePenalty),d.coOccurrencePenalty>0?"#cc8844":"#8090a0");
            addBR("━━ TOTAL SCORE ━━", fmt(d.totalScore),         d.totalScore>=0?"#88ff88":"#ff8888");

            bt->resizeRowsToContents();
            bt->setFixedHeight(bt->rowCount()*22+4);
            vl->addWidget(bt);

            // Per-rower table
            int nR = d.rowers.size();
            if (nR == 0) { auto* sep = new QFrame; sep->setFrameShape(QFrame::HLine); vl->addWidget(sep); continue; }

            vl->addWidget(new QLabel(QString("<span style='color:#6a8aaa;font-size:11px;'>Rower details (%1)</span>").arg(nR)));

            auto* rt = new QTableWidget(0, 1+nR);
            rt->verticalHeader()->setVisible(false);
            rt->setEditTriggers(QAbstractItemView::NoEditTriggers);
            rt->setSelectionMode(QAbstractItemView::NoSelection);
            rt->setStyleSheet("QTableWidget{background:#0a1520;gridline-color:#1a2538;}"
                              "QTableWidget::item{padding:3px 6px;}");
            rt->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
            for (int ci=1;ci<=nR;++ci) rt->horizontalHeader()->setSectionResizeMode(ci,QHeaderView::Stretch);
            rt->setHorizontalHeaderItem(0, new QTableWidgetItem("Parameter"));
            for (int i=0;i<nR;++i) {
                QString rn = QString("Rower#%1").arg(d.rowers[i].rowerId);
                for (const Rower& r:m_rowers) if(r.id()==d.rowers[i].rowerId){rn=r.name();break;}
                auto* h=new QTableWidgetItem(rn); QFont f=h->font();f.setBold(true);h->setFont(f);
                rt->setHorizontalHeaderItem(i+1,h);
            }

            const QString SL[]={"—","Short","Medium","Long"}, BS[]={"—","Small","Medium","Tall"};
            struct R{QString label;std::function<QString(const ScoreDetail::RowerDetail&)>fn;};
            QList<R> rowdefs;
            rowdefs.append(R{"Skill",      [](const ScoreDetail::RowerDetail& r){return QString::number(r.skillInt);}});
            rowdefs.append(R{"Propulsion", [](const ScoreDetail::RowerDetail& r){return r.propScore==1.0?"Exact":r.propScore==0.5?"Both":"None";}});
            rowdefs.append(R{"Compat",     [](const ScoreDetail::RowerDetail& r){return r.compatTier;}});
            rowdefs.append(R{"Strength",   [](const ScoreDetail::RowerDetail& r){return r.strength>0?QString::number(r.strength):QString("—");}});
            rowdefs.append(R{"Stroke Len", [SL](const ScoreDetail::RowerDetail& r){return r.strokeLength>=0&&r.strokeLength<=3?SL[r.strokeLength]:QString("—");}});
            rowdefs.append(R{"Body Size",  [BS](const ScoreDetail::RowerDetail& r){return r.bodySize>=0&&r.bodySize<=3?BS[r.bodySize]:QString("—");}});
            rowdefs.append(R{"GrpAttr1",   [](const ScoreDetail::RowerDetail& r){return r.attrGrp1>0?QString::number(r.attrGrp1):QString("—");}});
            rowdefs.append(R{"GrpAttr2",   [](const ScoreDetail::RowerDetail& r){return r.attrGrp2>0?QString::number(r.attrGrp2):QString("—");}});
            rowdefs.append(R{"ValAttr1",   [](const ScoreDetail::RowerDetail& r){return r.attrVal1>0?QString::number(r.attrVal1):QString("—");}});
            rowdefs.append(R{"ValAttr2",   [](const ScoreDetail::RowerDetail& r){return r.attrVal2>0?QString::number(r.attrVal2):QString("—");}});
            rowdefs.append(R{"Obmann",     [](const ScoreDetail::RowerDetail& r){return QString(r.isObmann?"Yes":"No");}});
            rowdefs.append(R{"CanSteer",   [](const ScoreDetail::RowerDetail& r){return QString(r.canSteer?"Yes":"No");}});
            rowdefs.append(R{"WL bonus",   [](const ScoreDetail::RowerDetail& r){return r.whitelistContrib>0?QString("+%1").arg(r.whitelistContrib,0,'f',2):QString("0");}});
            rowdefs.append(R{"CoOcc pen",  [](const ScoreDetail::RowerDetail& r){return r.coOccContrib>0?QString("−%1").arg(r.coOccContrib,0,'f',2):QString("0");}});

            rt->setRowCount(rowdefs.size());
            for (int ri=0;ri<rowdefs.size();++ri) {
                auto* lbl=new QTableWidgetItem(rowdefs[ri].label); lbl->setForeground(QColor("#8090a0"));
                rt->setItem(ri,0,lbl);
                for (int ci=0;ci<nR;++ci) rt->setItem(ri,ci+1,new QTableWidgetItem(rowdefs[ri].fn(d.rowers[ci])));
            }
            rt->resizeRowsToContents();
            rt->setFixedHeight(rt->rowCount()*22+26);
            vl->addWidget(rt);

            auto* sep=new QFrame; sep->setFrameShape(QFrame::HLine); sep->setStyleSheet("color:#2a3548;"); vl->addWidget(sep);
        }

        vl->addStretch();
        scroll->setWidget(inner);
        outerVL->addWidget(scroll);

        m_assignmentScoreWidget = outer;
        if (scoreTabIdx >= 0)
            m_assignmentViewTabs->insertTab(scoreTabIdx, m_assignmentScoreWidget,
                                            QString("Scoring (%1)").arg(details.size()));
        else
            m_assignmentViewTabs->addTab(m_assignmentScoreWidget,
                                         QString("Scoring (%1)").arg(details.size()));
    }
}

void MainWindow::onCopyToClipboard() {
    const auto& map = m_currentAssignment.boatRowerMap();
    QString clipText;
    clipText += m_currentAssignment.name() + "\n\n";

    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        clipText += QString("====== %1 ======\n").arg(boatDescription(it.key()));
        const QList<int>& rowerIds = it.value();

        Boat foundBoat;
        for (const Boat& b : m_boats)
            if (b.id() == it.key()) { foundBoat = b; break; }
        bool isSteered = foundBoat.id() != -1
                         && foundBoat.steeringType() == SteeringType::Steered;

        // Pick Obmann
        QList<int> obmannCandidates;
        for (int rid : rowerIds)
            for (const Rower& r : m_rowers)
                if (r.id() == rid && r.isObmann()) { obmannCandidates << rid; break; }
        int chosenObmann = -1;
        if (!obmannCandidates.isEmpty()) {
            int idx = obmannCandidates.size() == 1
                ? 0
                : static_cast<int>(QRandomGenerator::global()->bounded(
                      static_cast<quint32>(obmannCandidates.size())));
            chosenObmann = obmannCandidates.at(idx);
        }

        // Pick Steerer
        int chosenSteerer = -1;
        if (isSteered) {
            QList<int> steererCandidates;
            for (int rid : rowerIds)
                for (const Rower& r : m_rowers)
                    if (r.id() == rid && r.canSteer() && rid != chosenObmann)
                        { steererCandidates << rid; break; }
            if (steererCandidates.isEmpty())
                for (int rid : rowerIds)
                    for (const Rower& r : m_rowers)
                        if (r.id() == rid && r.canSteer())
                            { steererCandidates << rid; break; }
            if (!steererCandidates.isEmpty()) {
                int idx = steererCandidates.size() == 1
                    ? 0
                    : static_cast<int>(QRandomGenerator::global()->bounded(
                          static_cast<quint32>(steererCandidates.size())));
                chosenSteerer = steererCandidates.at(idx);
            }
        }

        QStringList lines;
        if (chosenObmann != -1) {
            QString tag = "[Obmann]";
            if (chosenObmann == chosenSteerer) tag += " [Steering]";
            lines << QString("%1  %2").arg(rowerName(chosenObmann)).arg(tag);
        }
        for (int rid : rowerIds) {
            if (rid == chosenObmann) continue;
            QStringList tags;
            if (rid == chosenSteerer) tags << "[Steering]";
            QString line = rowerName(rid);
            if (!tags.isEmpty()) line += "  " + tags.join(" ");
            lines << line;
        }
        clipText += lines.join("\n") + "\n\n";
    }

    QApplication::clipboard()->setText(clipText.trimmed());
    statusBar()->showMessage("Copied to clipboard!", 2500);
    m_copyBtn->setText("Copied!");
    QTimer::singleShot(2000, this, [this]() {
        m_copyBtn->setText("Copy to Clipboard");
    });
}

// ---------------------------------------------------------------
// Distance Tab
// ---------------------------------------------------------------
QWidget* MainWindow::buildDistanceTab() {
    auto* w = new QWidget;
    auto* vl = new QVBoxLayout(w);
    vl->setContentsMargins(12,12,12,12);
    vl->setSpacing(10);

    auto* info = new QLabel(
        "Select an assignment, then enter km for any rower — "
        "the same value auto-fills all others in that assignment. "
        "Individual edits are preserved independently.");
    info->setWordWrap(true);
    info->setStyleSheet("color:#5a7a9a; font-style:italic;");
    vl->addWidget(info);

    auto* row = new QHBoxLayout;
    row->addWidget(new QLabel("Assignment:"));
    m_distAssignmentCombo = new QComboBox;
    m_distAssignmentCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    row->addWidget(m_distAssignmentCombo, 1);
    vl->addLayout(row);

    m_distTable = new QTableWidget(0, 3);
    m_distTable->setHorizontalHeaderLabels({"Rower", "Boat", "km"});
    m_distTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_distTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_distTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_distTable->verticalHeader()->setVisible(false);
    m_distTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    vl->addWidget(m_distTable, 1);

    connect(m_distAssignmentCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onAssignmentDistanceSelected);
    connect(m_distTable, &QTableWidget::cellChanged,
            this, &MainWindow::onDistanceChanged);

    return w;
}

void MainWindow::onAssignmentDistanceSelected(int index) {
    if (!m_distTable || !m_distAssignmentCombo) return;
    m_distUpdating = true;
    m_distTable->setRowCount(0);
    if (index < 0) { m_distUpdating = false; return; }

    // Combo stores assignment ID in UserData
    int assignmentId = m_distAssignmentCombo->itemData(index).toInt();
    if (assignmentId <= 0) { m_distUpdating = false; return; }

    Assignment full = m_db->loadAssignment(assignmentId);
    QMap<int,int> distances = m_db->loadDistances(full.id());

    const auto& bmap = full.boatRowerMap();
    for (auto it = bmap.constBegin(); it != bmap.constEnd(); ++it) {
        QString bName = boatDescription(it.key());
        for (int rid : it.value()) {
            int row = m_distTable->rowCount();
            m_distTable->insertRow(row);

            auto* nameItem = new QTableWidgetItem(rowerName(rid));
            nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
            nameItem->setData(Qt::UserRole, rid);
            nameItem->setData(Qt::UserRole + 1, full.id());
            nameItem->setData(Qt::UserRole + 2, it.key()); // boatId
            m_distTable->setItem(row, 0, nameItem);

            auto* boatItem = new QTableWidgetItem(bName);
            boatItem->setFlags(boatItem->flags() & ~Qt::ItemIsEditable);
            m_distTable->setItem(row, 1, boatItem);

            int km = distances.value(rid, 0);
            auto* kmItem = new QTableWidgetItem(QString::number(km));
            m_distTable->setItem(row, 2, kmItem);
        }
    }
    m_distUpdating = false;
}

void MainWindow::onDistanceChanged(int row, int col) {
    if (col != 2 || m_distUpdating || !m_distTable) return;
    auto* kmItem   = m_distTable->item(row, 2);
    auto* nameItem = m_distTable->item(row, 0);
    if (!kmItem || !nameItem) return;

    bool ok;
    int km = kmItem->text().toInt(&ok);
    if (!ok || km < 0) return;

    int assignmentId = nameItem->data(Qt::UserRole + 1).toInt();
    int boatId       = nameItem->data(Qt::UserRole + 2).toInt();  // same boat only

    m_db->saveDistance(assignmentId, nameItem->data(Qt::UserRole).toInt(), km);

    // Auto-fill others in the SAME BOAT that still have 0 km
    m_distUpdating = true;
    for (int r = 0; r < m_distTable->rowCount(); ++r) {
        if (r == row) continue;
        auto* otherName = m_distTable->item(r, 0);
        auto* otherKm   = m_distTable->item(r, 2);
        if (!otherName || !otherKm) continue;
        // Must be same assignment AND same boat
        if (otherName->data(Qt::UserRole + 1).toInt() != assignmentId) continue;
        if (otherName->data(Qt::UserRole + 2).toInt() != boatId)       continue;
        if (otherKm->text().toInt() == 0) {
            otherKm->setText(QString::number(km));
            m_db->saveDistance(assignmentId, otherName->data(Qt::UserRole).toInt(), km);
        }
    }
    m_distUpdating = false;
}

// ---------------------------------------------------------------
// Statistics Tab
// ---------------------------------------------------------------
QWidget* MainWindow::buildStatsTab() {
    auto* w = new QWidget;
    auto* vl = new QVBoxLayout(w);
    vl->setContentsMargins(12,12,12,12);
    vl->setSpacing(10);

    auto* titleLabel = new QLabel("Rower Statistics — Obmann, Steering and Distance history");
    titleLabel->setStyleSheet("color:#8fb4d8; font-weight:700; font-size:13px;");
    vl->addWidget(titleLabel);

    auto* info = new QLabel(
        "Counts are lifetime totals. 'Recent' shows count in the last 3 sessions "
        "(used to reduce the weight of overused roles when assigning).");
    info->setWordWrap(true);
    info->setStyleSheet("color:#5a7a9a; font-style:italic;");
    vl->addWidget(info);

    m_statsTable = new QTableWidget(0, 7);
    m_statsTable->setHorizontalHeaderLabels({
        "Rower", "Obmann (total)", "Obmann (recent 3)",
        "Steering (total)", "Steering (recent 3)", "Total km", "Notes"
    });
    m_statsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int i = 1; i <= 6; ++i)
        m_statsTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    m_statsTable->verticalHeader()->setVisible(false);
    m_statsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_statsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_statsTable->setAlternatingRowColors(true);
    vl->addWidget(m_statsTable, 1);

    auto* refreshBtn = new QPushButton("Refresh");
    refreshBtn->setObjectName("primaryBtn");
    vl->addWidget(refreshBtn, 0, Qt::AlignLeft);
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshStats);

    return w;
}

void MainWindow::refreshStats() {
    if (!m_statsTable) return;
    m_statsTable->setRowCount(0);
    QList<DatabaseManager::RowerStats> stats = m_db->loadStats();
    for (const DatabaseManager::RowerStats& s : stats) {
        int row = m_statsTable->rowCount();
        m_statsTable->insertRow(row);
        m_statsTable->setItem(row, 0, new QTableWidgetItem(s.name));
        m_statsTable->setItem(row, 1, new QTableWidgetItem(QString::number(s.obmannCount)));
        m_statsTable->setItem(row, 2, new QTableWidgetItem(QString::number(s.recentObmann)));
        m_statsTable->setItem(row, 3, new QTableWidgetItem(QString::number(s.steeringCount)));
        m_statsTable->setItem(row, 4, new QTableWidgetItem(QString::number(s.recentSteering)));
        m_statsTable->setItem(row, 5, new QTableWidgetItem(QString::number(s.totalKm) + " km"));
        // Notes for overuse
        QStringList notes;
        if (s.recentObmann >= 3)   notes << "Obmann overused";
        if (s.recentSteering >= 3) notes << "Steering overused";
        m_statsTable->setItem(row, 6, new QTableWidgetItem(notes.join(", ")));
    }
}

// ---------------------------------------------------------------
// Distance Detail Tab — rowers × last 14 assignments, rotated headers
// ---------------------------------------------------------------
QWidget* MainWindow::buildDistanceDetailTab() {
    auto* w = new QWidget;
    auto* vl = new QVBoxLayout(w);
    vl->setContentsMargins(12, 12, 12, 12);
    vl->setSpacing(8);

    auto* info = new QLabel(
        "Kilometres per rower for the last 14 assignments. "
        "Assignment names are rotated in the column headers.");
    info->setWordWrap(true);
    info->setStyleSheet("color:#5a7a9a; font-style:italic;");
    vl->addWidget(info);

    m_distDetailTable = new QTableWidget(0, 0);
    m_distDetailTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_distDetailTable->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_distDetailTable->setAlternatingRowColors(true);
    vl->addWidget(m_distDetailTable, 1);

    auto* refreshBtn = new QPushButton("Refresh");
    refreshBtn->setObjectName("primaryBtn");
    vl->addWidget(refreshBtn, 0, Qt::AlignLeft);

    connect(refreshBtn, &QPushButton::clicked, this, [this]() {
        if (!m_distDetailTable) return;

        // Take the last 14 assignments (they are stored newest-first)
        const int MAX_ASSIGNMENTS = 14;
        QList<Assignment> recent;
        for (int i = 0; i < qMin(MAX_ASSIGNMENTS, m_assignments.size()); ++i)
            recent.append(m_assignments.at(i));
        // Reverse so oldest is leftmost
        std::reverse(recent.begin(), recent.end());

        if (recent.isEmpty()) { m_distDetailTable->setRowCount(0); m_distDetailTable->setColumnCount(0); return; }

        // Build row index: all rowers sorted by name
        QList<Rower> sortedRowers = m_rowers;
        std::sort(sortedRowers.begin(), sortedRowers.end(),
                  [](const Rower& a, const Rower& b){ return a.name() < b.name(); });

        m_distDetailTable->setRowCount(sortedRowers.size());
        m_distDetailTable->setColumnCount(recent.size() + 1); // +1 for Total

        // Vertical headers = rower names
        for (int r = 0; r < sortedRowers.size(); ++r)
            m_distDetailTable->setVerticalHeaderItem(r, new QTableWidgetItem(sortedRowers[r].name()));

        // Horizontal headers = assignment names (rotated via custom delegate)
        for (int c = 0; c < recent.size(); ++c) {
            const Assignment& a = recent.at(c);
            QString label = QString("%1\n(%2)").arg(a.name())
                                .arg(a.createdAt().toString("dd.MM."));
            auto* hItem = new QTableWidgetItem(label);
            hItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            m_distDetailTable->setHorizontalHeaderItem(c, hItem);
        }
        m_distDetailTable->setHorizontalHeaderItem(recent.size(), new QTableWidgetItem("Total km"));

        // Fill data
        for (int r = 0; r < sortedRowers.size(); ++r) {
            int rowerId = sortedRowers[r].id();
            int total = 0;
            for (int c = 0; c < recent.size(); ++c) {
                Assignment full = m_db->loadAssignment(recent[c].id());
                QMap<int,int> dist = m_db->loadDistances(full.id());
                int km = dist.value(rowerId, 0);
                total += km;
                auto* cell = new QTableWidgetItem(km > 0 ? QString::number(km) : "");
                cell->setTextAlignment(Qt::AlignCenter);
                m_distDetailTable->setItem(r, c, cell);
            }
            auto* totalCell = new QTableWidgetItem(total > 0 ? QString("%1 km").arg(total) : "");
            totalCell->setTextAlignment(Qt::AlignCenter);
            if (total > 0) totalCell->setForeground(QColor("#66ff99"));
            m_distDetailTable->setItem(r, recent.size(), totalCell);
        }

        // Rotate column headers by setting a custom stylesheet trick:
        // We use a fixed height for the header and rotate text via a style
        m_distDetailTable->horizontalHeader()->setMinimumSectionSize(50);
        m_distDetailTable->horizontalHeader()->setDefaultSectionSize(70);
        m_distDetailTable->horizontalHeader()->setMaximumSectionSize(70);
        m_distDetailTable->horizontalHeader()->setFixedHeight(120);
        m_distDetailTable->setStyleSheet(m_distDetailTable->styleSheet() +
            "QHeaderView::section { padding: 4px; }");

        // Apply rotation via custom delegate (inline)
        class RotatedHeaderDelegate : public QStyledItemDelegate {
        public:
            using QStyledItemDelegate::QStyledItemDelegate;
            void paint(QPainter* painter, const QStyleOptionViewItem& option,
                       const QModelIndex& index) const override {
                painter->save();
                painter->translate(option.rect.bottomLeft());
                painter->rotate(-70);
                QRect r(0, 0, option.rect.height(), option.rect.width());
                painter->setPen(QColor("#e8edf5"));
                painter->drawText(r, Qt::AlignLeft | Qt::AlignVCenter,
                    index.data().toString());
                painter->restore();
            }
            QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override {
                return QSize(70, 120);
            }
        };
        m_distDetailTable->horizontalHeader()->setItemDelegate(
            new RotatedHeaderDelegate(m_distDetailTable->horizontalHeader()));

        m_distDetailTable->resizeRowsToContents();
    });

    return w;
}

// ---------------------------------------------------------------
// Options Tab (main window) — sick mode
// ---------------------------------------------------------------
QWidget* MainWindow::buildOptionsTab() {
    auto* w = new QWidget;
    auto* vl = new QVBoxLayout(w);
    vl->setContentsMargins(12, 12, 12, 12);
    vl->setSpacing(12);

    auto* titleLabel = new QLabel("Options");
    titleLabel->setStyleSheet("color:#8fb4d8; font-weight:700; font-size:14px;");
    vl->addWidget(titleLabel);

    // ---- Equipment limits ----
    auto* equipGroup = new QGroupBox("Equipment limits  (0 = no limit)");
    auto* equipVL    = new QVBoxLayout(equipGroup);
    auto* equipInfo  = new QLabel(
        "Set the number of oars available globally. Each rower in a scull boat uses "
        "one scull oar; each rower in a sweep boat uses one sweep oar. "
        "The assignment dialog enforces these limits when checking and generating.");
    equipInfo->setWordWrap(true);
    equipInfo->setStyleSheet("color:#5a7a9a; font-style:italic;");
    equipVL->addWidget(equipInfo);

    auto* skullRow = new QHBoxLayout;
    skullRow->addWidget(new QLabel("Scull oars available:"));
    m_scullOarsSpinBox = new QSpinBox;
    m_scullOarsSpinBox->setRange(0, 500);
    m_scullOarsSpinBox->setSpecialValueText("No limit");
    m_scullOarsSpinBox->setValue(0);
    skullRow->addWidget(m_scullOarsSpinBox);
    skullRow->addStretch();
    equipVL->addLayout(skullRow);

    auto* sweepRow = new QHBoxLayout;
    sweepRow->addWidget(new QLabel("Sweep oars available:"));
    m_sweepOarsSpinBox = new QSpinBox;
    m_sweepOarsSpinBox->setRange(0, 500);
    m_sweepOarsSpinBox->setSpecialValueText("No limit");
    m_sweepOarsSpinBox->setValue(0);
    sweepRow->addWidget(m_sweepOarsSpinBox);
    sweepRow->addStretch();
    equipVL->addLayout(sweepRow);
    vl->addWidget(equipGroup);

    // ---- Sick mode ----
    auto* sickGroup = new QGroupBox("Sick / Unavailable Rowers");
    auto* sickVL    = new QVBoxLayout(sickGroup);

    auto* sickInfo = new QLabel(
        "Rowers marked as sick are excluded from the assignment generator and the "
        "rower selection in all dialogs. They remain in the database for statistics.\n"
        "Toggle sick status by checking/unchecking the checkbox next to each name.");
    sickInfo->setWordWrap(true);
    sickInfo->setStyleSheet("color:#5a7a9a; font-style:italic;");
    sickVL->addWidget(sickInfo);

    m_sickTable = new QTableWidget(0, 2);
    m_sickTable->setHorizontalHeaderLabels({"Rower", "Sick / Unavailable"});
    m_sickTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_sickTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_sickTable->verticalHeader()->setVisible(false);
    m_sickTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_sickTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    sickVL->addWidget(m_sickTable, 1);

    auto* refreshSickBtn = new QPushButton("Refresh list");
    sickVL->addWidget(refreshSickBtn, 0, Qt::AlignLeft);
    vl->addWidget(sickGroup, 1);
    vl->addStretch();

    connect(refreshSickBtn, &QPushButton::clicked, this, &MainWindow::refreshSickList);
    connect(m_sickTable, &QTableWidget::cellChanged, this, [this](int row, int col) {
        if (col != 1 || !m_sickTable) return;
        auto* nameItem = m_sickTable->item(row, 0);
        auto* sickItem = m_sickTable->item(row, 1);
        if (!nameItem || !sickItem) return;
        int rowerId = nameItem->data(Qt::UserRole).toInt();
        bool sick   = sickItem->checkState() == Qt::Checked;
        m_db->setSickRower(rowerId, sick);
        if (sick && !m_sickRowerIds.contains(rowerId))
            m_sickRowerIds.append(rowerId);
        else if (!sick)
            m_sickRowerIds.removeAll(rowerId);
    });

    // Initial population
    QTimer::singleShot(0, this, &MainWindow::refreshSickList);
    return w;
}

void MainWindow::refreshSickList() {
    if (!m_sickTable) return;
    m_sickTable->blockSignals(true);
    m_sickTable->setRowCount(0);
    m_sickRowerIds = m_db->loadSickRowerIds();
    QList<Rower> sorted = m_rowers;
    std::sort(sorted.begin(), sorted.end(),
              [](const Rower& a, const Rower& b){ return a.name() < b.name(); });
    for (const Rower& r : sorted) {
        int row = m_sickTable->rowCount();
        m_sickTable->insertRow(row);
        auto* nameItem = new QTableWidgetItem(r.name());
        nameItem->setData(Qt::UserRole, r.id());
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        m_sickTable->setItem(row, 0, nameItem);
        auto* sickItem = new QTableWidgetItem;
        sickItem->setFlags(sickItem->flags() | Qt::ItemIsUserCheckable);
        sickItem->setCheckState(m_sickRowerIds.contains(r.id()) ? Qt::Checked : Qt::Unchecked);
        sickItem->setTextAlignment(Qt::AlignCenter);
        m_sickTable->setItem(row, 1, sickItem);
    }
    m_sickTable->blockSignals(false);
}

void MainWindow::onSickModeChanged(int rowerId, bool sick) {
    m_db->setSickRower(rowerId, sick);
    if (sick && !m_sickRowerIds.contains(rowerId))  m_sickRowerIds.append(rowerId);
    else if (!sick) m_sickRowerIds.removeAll(rowerId);
}

// ---------------------------------------------------------------
// Print assignment
// ---------------------------------------------------------------
void MainWindow::onPrintAssignment()
{
    if (m_currentAssignment.boatRowerMap().isEmpty()) {
        statusBar()->showMessage("No assignment to print.", 2000);
        return;
    }

    int copies = m_printCopiesSpinBox ? m_printCopiesSpinBox->value() : 1;

    if (!m_printer.isConnected()) {
        statusBar()->showMessage("Searching for printer...", 0);
        if (!m_printer.findAndConnect()) {
            QMessageBox::warning(this, "Printer Not Found",
                QString("Could not connect to printer:\n\n%1")
                    .arg(m_printer.statusMessage()));
            statusBar()->showMessage("Print failed: no printer.", 3000);
            return;
        }
        statusBar()->showMessage("Printer connected: " + m_printer.deviceDesc(), 2000);
    }

    // Use EXACTLY the same text shown in the Text view — roles are loaded
    // from the database, so printed output is always identical to screen output.
    QString printText = formatAssignmentText(m_currentAssignment);

    bool success = true;
    for (int copy = 0; copy < copies && success; ++copy) {
        statusBar()->showMessage(QString("Printing copy %1 of %2...").arg(copy+1).arg(copies), 0);
        success = m_printer.printText(printText);
    }

    if (!success) {
        QMessageBox::warning(this, "Print Error",
            QString("Printing failed:\n\n%1").arg(m_printer.statusMessage()));
        statusBar()->showMessage("Print failed.", 3000);
    } else {
        statusBar()->showMessage(
            QString("Printed %1 copy(s).").arg(copies), 3000);
    }
}

// ---------------------------------------------------------------
// Table view for assignments (Excel-style: one column per boat)
// ---------------------------------------------------------------
void MainWindow::populateAssignmentTable(const Assignment& assignment) {
    if (!m_assignmentTable) return;
    m_assignmentTable->clear();
    m_assignmentTable->setRowCount(0);
    m_assignmentTable->setColumnCount(0);

    const auto& bmap = assignment.boatRowerMap();
    if (bmap.isEmpty()) return;

    // Load saved roles so we can annotate without re-rolling
    QMap<int,QString> savedRoles = m_db->loadRoles(assignment.id());

    // Build column list in map order
    QList<int> boatIds = bmap.keys();
    int numCols = boatIds.size();

    // Find max rower count across all boats for row count
    int maxRowers = 0;
    for (int bid : boatIds)
        maxRowers = qMax(maxRowers, bmap[bid].size());

    // +1 row for boat metadata (type/cap/steering/propulsion)
    const int ROW_META  = 0;
    const int ROW_START = 1;

    m_assignmentTable->setColumnCount(numCols);
    m_assignmentTable->setRowCount(ROW_START + maxRowers);
    m_assignmentTable->horizontalHeader()->setVisible(true);
    m_assignmentTable->verticalHeader()->setVisible(false);

    // Column headers = boat names
    for (int c = 0; c < numCols; ++c) {
        int boatId = boatIds.at(c);
        Boat foundBoat;
        for (const Boat& b : m_boats) if (b.id() == boatId) { foundBoat = b; break; }
        QString header = foundBoat.id() != -1 ? foundBoat.name() : QString("Boat#%1").arg(boatId);
        m_assignmentTable->setHorizontalHeaderItem(c, new QTableWidgetItem(header));
    }

    // Row 0: boat metadata
    for (int c = 0; c < numCols; ++c) {
        int boatId = boatIds.at(c);
        Boat foundBoat;
        for (const Boat& b : m_boats) if (b.id() == boatId) { foundBoat = b; break; }
        QString meta;
        if (foundBoat.id() != -1)
            meta = QString("%1 | Cap:%2 | %3 | %4")
                .arg(Boat::boatTypeToString(foundBoat.boatType()))
                .arg(foundBoat.capacity())
                .arg(Boat::steeringTypeToString(foundBoat.steeringType()))
                .arg(Boat::propulsionTypeToString(foundBoat.propulsionType()));
        auto* metaItem = new QTableWidgetItem(meta);
        metaItem->setForeground(QColor("#5a7a9a"));
        QFont f = metaItem->font(); f.setItalic(true); metaItem->setFont(f);
        m_assignmentTable->setItem(ROW_META, c, metaItem);
    }

    // Rower rows
    for (int c = 0; c < numCols; ++c) {
        int boatId = boatIds.at(c);
        const QList<int>& rowerIds = bmap[boatId];

        // Find Obmann first (to print first)
        int obmannId = -1;
        for (int rid : rowerIds) {
            QString role = savedRoles.value(rid);
            if (role == "obmann" || role == "obmann_steering") { obmannId = rid; break; }
        }

        // Build ordered list: obmann first, rest in original order
        QList<int> ordered;
        if (obmannId != -1) ordered << obmannId;
        for (int rid : rowerIds) if (rid != obmannId) ordered << rid;

        for (int r = 0; r < ordered.size(); ++r) {
            int rid = ordered.at(r);
            QString name;
            for (const Rower& ro : m_rowers) if (ro.id() == rid) { name = ro.name(); break; }
            if (name.isEmpty()) name = QString("Rower#%1").arg(rid);

            // Append role tag
            QString role = savedRoles.value(rid);
            if (role == "obmann")         name += "  [Obmann]";
            else if (role == "steering")  name += "  [Steering]";
            else if (role == "obmann_steering") name += "  [Obmann][Steering]";

            auto* cell = new QTableWidgetItem(name);

            // Highlight Obmann row
            if (role == "obmann" || role == "obmann_steering") {
                cell->setForeground(QColor("#f0c060"));
                QFont f = cell->font(); f.setBold(true); cell->setFont(f);
            } else if (role == "steering") {
                cell->setForeground(QColor("#60c0f0"));
            }

            m_assignmentTable->setItem(ROW_START + r, c, cell);
        }
    }

    m_assignmentTable->resizeColumnsToContents();
    m_assignmentTable->horizontalHeader()->setStretchLastSection(true);
}

// ---------------------------------------------------------------
// Boat Whitelist / Blacklist editors
// ---------------------------------------------------------------
static void editBoatList(
    QWidget* parent, const QString& title, const QList<Boat>& allBoats,
    QList<int>& currentList, std::function<void()> onSave)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(title);
    auto* vl = new QVBoxLayout(&dlg);
    auto* info = new QLabel(title.contains("Whitelist")
        ? "The rower prefers ONLY these boats (any one of them is acceptable).\nIf empty, no boat preference."
        : "The rower refuses to row in any of these boats.");
    info->setWordWrap(true);
    vl->addWidget(info);
    QVector<QCheckBox*> cbs;
    for (const Boat& b : allBoats) {
        auto* cb = new QCheckBox(QString("%1  [Cap:%2|%3|%4]")
            .arg(b.name()).arg(b.capacity())
            .arg(Boat::steeringTypeToString(b.steeringType()))
            .arg(Boat::propulsionTypeToString(b.propulsionType())));
        cb->setProperty("boatId", b.id());
        cb->setChecked(currentList.contains(b.id()));
        vl->addWidget(cb);
        cbs.append(cb);
    }
    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    vl->addWidget(btns);
    QObject::connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() == QDialog::Accepted) {
        currentList.clear();
        for (auto* cb : cbs)
            if (cb->isChecked()) currentList.append(cb->property("boatId").toInt());
        onSave();
    }
}


// ---------------------------------------------------------------
// Lock / unlock assignment
// ---------------------------------------------------------------
void MainWindow::onToggleLockAssignment() {
    auto* item = m_assignmentList->currentItem();
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    for (Assignment& a : m_assignments) {
        if (a.id() != id) continue;
        bool newLocked = !a.isLocked();
        // Unlocking requires password; locking does not
        if (!newLocked && !checkPassword("unlock this assignment")) return;
        a.setLocked(newLocked);
        m_db->setAssignmentLocked(id, newLocked);
        QString prefix = newLocked ? "🔒 " : "";
        item->setText(prefix + QString("%1\n%2")
            .arg(a.name())
            .arg(a.createdAt().toString("dd.MM.yyyy hh:mm")));
        if (m_currentAssignment.id() == id) m_currentAssignment.setLocked(newLocked);
        // Refresh distance combo so only locked assignments appear
        refreshAssignmentList();
        statusBar()->showMessage(
            newLocked ? "Assignment locked." : "Assignment unlocked.", 3000);
        break;
    }
}

// ---------------------------------------------------------------
// Expert Settings Tab
// ---------------------------------------------------------------
QWidget* MainWindow::buildExpertTab() {
    auto* outer = new QWidget;
    auto* outerVL = new QVBoxLayout(outer);
    outerVL->setContentsMargins(0, 0, 0, 0);
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* w  = new QWidget;
    auto* vl = new QVBoxLayout(w);
    vl->setContentsMargins(16, 16, 16, 24);
    vl->setSpacing(20);

    // ── Style helpers ────────────────────────────────────────────
    // Shared state: all spinboxes disabled until password is entered
    QVector<QWidget*>* spinboxes = new QVector<QWidget*>();
    bool* unlocked = new bool(false);
    auto mkHeader = [](const QString& t) {
        auto* l = new QLabel(t);
        l->setStyleSheet("color:#8fb4d8; font-weight:700; font-size:13px; "
                         "border-bottom:1px solid #2a3548; padding-bottom:4px; margin-top:6px;");
        return l;
    };
    auto mkFormula = [](const QString& t) {
        auto* l = new QLabel(t);
        l->setWordWrap(true);
        l->setStyleSheet("font-family:monospace; color:#a0c8e8; font-size:12px; "
                         "background:#0a1520; padding:4px 8px; border-radius:3px;");
        return l;
    };
    auto mkVarDesc = [](const QString& t) {
        auto* l = new QLabel(t);
        l->setWordWrap(true);
        l->setStyleSheet("color:#8090a0; font-size:11px; padding-left:4px;");
        return l;
    };
    auto mkWhenToUse = [](const QString& t) {
        auto* l = new QLabel("<b style='color:#5a9a5a;'>When to change:</b> " + t);
        l->setWordWrap(true);
        l->setStyleSheet("color:#4a8a4a; font-size:11px; padding-left:4px;");
        return l;
    };
    auto mkEffect = [](const QString& t) {
        auto* l = new QLabel("<b style='color:#7a6020;'>Effect of increasing:</b> " + t);
        l->setWordWrap(true);
        l->setStyleSheet("color:#7a6020; font-size:11px; padding-left:4px;");
        return l;
    };

    auto mkEffectDec = [](const QString& t) {
        auto* l = new QLabel("<b style='color:#205070;'>Effect of decreasing:</b> " + t);
        l->setWordWrap(true);
        l->setStyleSheet("color:#205070; font-size:11px; padding-left:4px;");
        return l;
    };

    // Spinbox builder for doubles
    auto mkDbl = [&](QWidget* parent, QVBoxLayout* pvl,
                     const QString& varName,
                     const QString& formula,
                     const QString& whatItIs,
                     const QString& whenToUse,
                     const QString& effectOfIncreasing,
                     const QString& effectOfDecreasing,
                     const QString& tooltip,
                     double val, double lo, double hi, double step,
                     std::function<void(double)> onChanged) {
        pvl->addWidget(new QLabel(
            "<span style='color:#c8d8e8; font-weight:600;'>" + varName + "</span>"
            "<span style='color:#556677; font-size:10px;'>  (default: " + QString::number(val, 'g', 4) + ")</span>"
        ));
        pvl->addWidget(mkFormula(formula));
        pvl->addWidget(mkVarDesc(whatItIs));
        pvl->addWidget(mkWhenToUse(whenToUse));
        pvl->addWidget(mkEffect(effectOfIncreasing));
        pvl->addWidget(mkEffectDec(effectOfDecreasing));
        auto* row = new QHBoxLayout;
        auto* spin = new QDoubleSpinBox;
        spin->setRange(lo, hi);
        spin->setSingleStep(step);
        spin->setDecimals(2);
        spin->setValue(val);
        spin->setToolTip(tooltip);
        spin->setMinimumWidth(90);
        spin->setMaximumWidth(110);
        spin->setEnabled(false);
        spinboxes->append(spin);
        row->addWidget(spin);
        row->addStretch();
        pvl->addLayout(row);
        pvl->addSpacing(10);
        QObject::connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                         parent, [onChanged, unlocked](double v){ if (*unlocked) onChanged(v); });
    };

    auto mkInt = [&](QWidget* parent, QVBoxLayout* pvl,
                     const QString& varName,
                     const QString& formula,
                     const QString& whatItIs,
                     const QString& whenToUse,
                     const QString& effectOfIncreasing,
                     const QString& effectOfDecreasing,
                     const QString& tooltip,
                     int val, int lo, int hi,
                     std::function<void(int)> onChanged) {
        pvl->addWidget(new QLabel(
            "<span style='color:#c8d8e8; font-weight:600;'>" + varName + "</span>"
            "<span style='color:#556677; font-size:10px;'>  (default: " + QString::number(val) + ")</span>"
        ));
        pvl->addWidget(mkFormula(formula));
        pvl->addWidget(mkVarDesc(whatItIs));
        pvl->addWidget(mkWhenToUse(whenToUse));
        pvl->addWidget(mkEffect(effectOfIncreasing));
        pvl->addWidget(mkEffectDec(effectOfDecreasing));
        auto* row = new QHBoxLayout;
        auto* spin = new QSpinBox;
        spin->setRange(lo, hi);
        spin->setValue(val);
        spin->setToolTip(tooltip);
        spin->setMinimumWidth(90);
        spin->setMaximumWidth(110);
        spin->setEnabled(false);
        spinboxes->append(spin);
        row->addWidget(spin);
        row->addStretch();
        pvl->addLayout(row);
        pvl->addSpacing(10);
        QObject::connect(spin, QOverload<int>::of(&QSpinBox::valueChanged),
                         parent, [onChanged, unlocked](int v){ if (*unlocked) onChanged(v); });
    };

    // ── Password lock banner ─────────────────────────────────────
    auto* lockBanner = new QWidget;
    lockBanner->setStyleSheet("background:#1a0a00; border-radius:6px; border:1px solid #7a3010;");
    auto* bannerHL = new QHBoxLayout(lockBanner);
    bannerHL->setContentsMargins(10, 8, 10, 8);
    auto* bannerLbl = new QLabel("🔒  Expert settings are locked. Enter the password to enable editing.");
    bannerLbl->setStyleSheet("color:#f0a060; font-size:12px;");
    auto* unlockBtn = new QPushButton("Unlock");
    unlockBtn->setMaximumWidth(90);
    unlockBtn->setObjectName("primaryBtn");
    bannerHL->addWidget(bannerLbl, 1);
    bannerHL->addWidget(unlockBtn);
    outerVL->addWidget(lockBanner);

    QObject::connect(unlockBtn, &QPushButton::clicked, w, [this, lockBanner, unlockBtn, bannerLbl, spinboxes, unlocked]() {
        if (*unlocked) return;
        if (!checkPassword("unlock expert settings")) return;
        *unlocked = true;
        lockBanner->setStyleSheet("background:#001a00; border-radius:6px; border:1px solid #207020;");
        bannerLbl->setText("🔓  Expert settings unlocked. Changes are saved immediately.");
        bannerLbl->setStyleSheet("color:#60c060; font-size:12px;");
        unlockBtn->setEnabled(false);
        for (QWidget* sb : *spinboxes) sb->setEnabled(true);
    });

    // ── Intro panel ──────────────────────────────────────────────
    auto* intro = new QLabel(
        "<b>Expert Settings</b> — tune every numeric constant in the assignment generator.<br><br>"
        "Each setting shows: its <b>variable name</b>, the <b>formula</b> it appears in, "
        "a plain-English description of <b>what the variable represents</b>, "
        "<b style='color:#5a9a5a;'>when you should change it</b>, "
        "and <b style='color:#7a6020;'>what increasing it does</b>.<br><br>"
        "<b>How scoring works in brief:</b> The generator tries up to <i>fillBoatAttempts</i> "
        "random team compositions per boat per pass. Each candidate team receives a <b>score</b>. "
        "The highest-scoring valid team is kept. Bonuses (positive terms) make the generator "
        "prefer a composition; penalties (negative terms) discourage it. "
        "Hard constraints reject a candidate entirely without scoring it.<br><br>"
        "<b>Key variable meanings:</b><br>"
        "<tt>score</tt> — total quality score for a candidate team (higher = generator prefers it)<br>"
        "<tt>gap</tt> — absolute difference between two rowers' attribute values (e.g. |Short−Long| = 2)<br>"
        "<tt>cap</tt> — boat capacity in rowing seats (not counting the foot-steerer)<br>"
        "<tt>variance</tt> — mean squared deviation: Σ(xᵢ − x̄)² / N. Large variance = unbalanced team.<br>"
        "<tt>cnt</tt> — how many past assignments a pair of rowers shared the same boat<br>"
        "<tt>w</tt> — priority weight, set by the rank position in Tab 3 of the assignment dialog<br>"
        "Changes are saved to the database immediately and survive restarts. "
        "Use <b>Reset to defaults</b> at the bottom to restore all original values."
    );
    intro->setWordWrap(true);
    intro->setStyleSheet("color:#8fb4d8; padding:10px; background:#0a1520; border-radius:6px; "
                         "border:1px solid #1a2a40;");
    vl->addWidget(intro);

    // ══ 1. PRIORITY WEIGHTS ═════════════════════════════════════
    vl->addWidget(mkHeader("1 — Priority Factor Weights  (w₁ … w₅)"));
    {
        auto* g = new QGroupBox;
        auto* gl = new QVBoxLayout(g);
        gl->addWidget(new QLabel(
            "<b>Context:</b> In the assignment dialog Tab 3 you drag five factors into a ranked list: "
            "Skill, Compatibility, Propulsion, Stroke Length, Body Size. "
            "The factor at rank 1 is multiplied by w₁, rank 2 by w₂, and so on. "
            "The product is added to the team score. "
            "The default ratio 4:2:1:0.5:0.5 means the top factor matters 8× more than the bottom two."
        ));
        gl->addWidget(mkFormula("score += wᵢ × factorScore(factor_at_rank_i)"));
        gl->addSpacing(8);

        struct { const char* name; double* ptr; double def; const char* whenTo; const char* effectInc; const char* effectDec; } ranks[] = {
            {"w₁  —  Weight for the rank-1 factor (top priority)",
             &m_expert.weightRank1, 4.0,
             "Increase if the top factor should completely dominate all others. "
             "Decrease if you want a more balanced competition between all factors.",
             "The rank-1 factor (e.g. Skill) becomes even more decisive. "
             "A gap of 1 skill level between two teams will matter more than any other difference.",
             "The top factor becomes less dominant. Other factors can now compete more equally "
             "with it. At w₁ = w₂ both rank-1 and rank-2 factors are equally weighted."},
            {"w₂  —  Weight for the rank-2 factor",
             &m_expert.weightRank2, 2.0,
             "Increase if rank-2 and rank-1 should have similar influence. "
             "Decrease if you want rank-1 to stand alone.",
             "The second factor gains influence. At w₂ = w₁, both factors are equally weighted.",
             "The second factor becomes a weaker contributor. Rank-1 increasingly dominates. "
             "At 0, the second factor has no effect at all on team selection."},
            {"w₃  —  Weight for the rank-3 factor",
             &m_expert.weightRank3, 1.0,
             "Increase if propulsion or whichever factor is at rank 3 is genuinely important. "
             "Often left low since Skill and Compatibility are usually more important.",
             "The third factor becomes more competitive with the top two.",
             "The third factor has even less influence on team selection. "
             "At 0, only the top two factors matter."},
            {"w₄  —  Weight for the rank-4 factor",
             &m_expert.weightRank4, 0.5,
             "Increase if Stroke Length or Body Size (typically at rank 4–5) should actually "
             "influence placement. Decrease toward 0 to treat them as pure tie-breakers.",
             "The fourth factor nudges team selection more noticeably.",
             "The fourth factor becomes an even weaker tie-breaker. "
             "At 0, it is completely ignored."},
            {"w₅  —  Weight for the rank-5 factor (lowest priority)",
             &m_expert.weightRank5, 0.5,
             "Rarely needs changing. At 0.5 it acts as a soft tie-breaker. "
             "Set to 0 to ignore the lowest factor entirely.",
             "The bottom factor has slightly more influence. Rarely meaningful.",
             "The lowest-ranked factor is ignored completely. "
             "All scoring comes from the top four factors."},
        };
        const QString rankKeys[] = {"weightRank1","weightRank2","weightRank3","weightRank4","weightRank5"};
        for (int i = 0; i < 5; ++i) {
            double* ptr = ranks[i].ptr;
            QString key = rankKeys[i];
            mkDbl(w, gl, ranks[i].name,
                  QString("score += w%1 × factorScore   (factor at rank %1 in priority list)").arg(i+1),
                  QString("w%1 is the multiplier applied to the score contribution of whichever "
                          "factor you placed at rank %1 in Tab 3. Higher = that factor contributes more.").arg(i+1),
                  ranks[i].whenTo, ranks[i].effectInc, ranks[i].effectDec,
                  QString("Default: %1. Range 0–10.").arg(ranks[i].def),
                  *ptr, 0.0, 10.0, 0.5,
                  [this, ptr, key](double v){ *ptr = v; m_db->saveExpertSetting(key, v); });
        }
        vl->addWidget(g);
    }

    // ══ 2 — Whitelist Bonus & Co-occurrence Penalty ═════════
    vl->addWidget(mkHeader("2 — Whitelist Bonus & Co-occurrence Penalty"));
    {
        auto* g = new QGroupBox;
        auto* gl = new QVBoxLayout(g);
        mkDbl(w, gl, "whitelistBonus  —  Bonus per whitelisted pair in the same boat",
              "score += whitelistBonus    (for each (rower_i, rower_j) pair where j ∈ whitelist(i))",
              "whitelistBonus is added to the team score for every pair of rowers that are both in the team AND one has the other on their personal whitelist (set in the Rowers tab). Note: if both A→B and B→A are set, the bonus is counted twice (once per direction).",
              "Increase when whitelist pairings should be treated as near-mandatory. Decrease when whitelists are soft preferences that should yield to Skill or Compat. Set to 0 to completely ignore whitelist entries during generation.",
              "Pairs with mutual whitelisting will dominate team selection. At very high values (>15) the generator may cluster whitelisted rowers so aggressively that skill balance and compatibility suffer.",
              "Whitelist preferences become weaker and eventually ignored. The generator treats whitelisted pairs the same as any other pair. At 0, whitelist entries in the Rowers tab have no effect at all on generation.",
              "Default: 5.0. Each direction of whitelist adds this bonus independently.",
              5.0, 0.0, 30.0, 0.5,
              [this](double v){ m_expert.whitelistBonus = v; m_db->saveExpertSetting("whitelistBonus", v); });
        mkDbl(w, gl, "coOccurrenceFactor  —  Penalty per past session a pair shared a boat",
              "score −= coOccurrenceFactor × cnt    (per pair, where cnt = past shared-boat sessions)",
              "coOccurrenceFactor controls how much the generator disfavours pairing rowers who have frequently been in the same boat before. cnt is loaded from the full assignment history in the database. A pair that has shared 5 sessions pays 5 × coOccurrenceFactor.",
              "Increase when you want the generator to actively rotate pairings across sessions — useful for a club that wants everyone to row with everyone else over time. Decrease when session-to-session consistency is more important than variety. Set to 0 to ignore co-occurrence history entirely.",
              "Frequently paired rowers are more aggressively separated. At very high values (>5) the generator may split up technically well-matched pairs just to achieve variety.",
              "Pairs that rowed together many times are penalised less and the generator is more willing to repeat pairings. At 0, the entire session history is ignored and the same pairs may be placed together every session.",
              "Default: 1.5. A pair with cnt=4 (4 shared sessions) pays 6.0 penalty.",
              1.5, 0.0, 10.0, 0.5,
              [this](double v){ m_expert.coOccurrenceFactor = v; m_db->saveExpertSetting("coOccurrenceFactor", v); });
        vl->addWidget(g);
    }

    // ══ 3 — Obmann Presence Bonus ═════════
    vl->addWidget(mkHeader("3 — Obmann Presence Bonus"));
    {
        auto* g = new QGroupBox;
        auto* gl = new QVBoxLayout(g);
        mkDbl(w, gl, "obmannBonus  —  Flat bonus for having an Obmann-capable rower in the team",
              "score += obmannBonus    (if any member has isObmann=true, boat cap > 2 only)",
              "obmannBonus is a flat score bonus added whenever at least one rower with the Obmann checkbox ticked (Rowers tab) is placed in a boat with more than 2 seats. It is a team-level bonus: having two Obmann-capable rowers gives the same bonus as one. The bonus must be large enough to outweigh skill and\n"
              "compatibility differences so the generator reliably places an Obmann in every boat.",
              "Increase if boats frequently end up without an Obmann despite qualified rowers being available. Decrease if you want the generator to sometimes skip Obmann placement when a significantly better composition is available without one. Set to 0 to treat Obmann purely as a display label.",
              "The generator becomes more aggressive about placing Obmann-capable rowers. Above ~25 it becomes near-mandatory, overriding most other soft factors. The typical skill contribution per boat is 4–16, so values above 20 already dominate.",
              "Obmann placement becomes optional. The generator may prefer a better skill or compat combination even if it means a boat has no Obmann. At 0, the Obmann flag has no influence on team selection; Obmann is only a display role chosen after placement.",
              "Default: 20.0. Must exceed typical skill+compat score (4–16) to reliably work.",
              20.0, 0.0, 50.0, 1.0,
              [this](double v){ m_expert.obmannBonus = v; m_db->saveExpertSetting("obmannBonus", v); });
        vl->addWidget(g);
    }

    // ══ 4 — Racing Boat / Beginner Soft Penalty ═════════
    vl->addWidget(mkHeader("4 — Racing Boat / Beginner Soft Penalty"));
    {
        auto* g = new QGroupBox;
        auto* gl = new QVBoxLayout(g);
        gl->addWidget(new QLabel("<b>Context:</b> In the first generation pass (strict mode), Student and Beginner rowers are HARD-blocked from Racing boats — no amount of scoring can place them there. In passes 2 and 3 (relaxed mode), the hard block is lifted and this soft penalty is the only thing discouraging the placement. The\n"
              "penalty is applied per beginner per boat evaluation."));
        gl->addSpacing(6);
        mkDbl(w, gl, "racingBeginnerPenalty  —  Soft penalty per beginner placed in a Racing boat",
              "score −= racingBeginnerPenalty    (per Student or Beginner rower in a Racing boat)",
              "racingBeginnerPenalty is subtracted from the team score for each rower whose skill level is Student or Beginner in a Racing-type boat. This applies in all generation passes including relaxed passes (2 and 3), unlike the hard block which only applies in pass 0.",
              "Increase if beginners still appear in Racing boats during relaxed passes. Decrease if your club allows beginners in Racing boats and you want less resistance. Set to 0 if boat type should have no influence on skill-based placement.",
              "Beginners are even more strongly pushed away from Racing boats in relaxed passes. At very high values (>20) it effectively becomes a second hard block in relaxed mode.",
              "Beginners placed in Racing boats during relaxed passes cost less in score. The generator accepts them more readily when other constraints are tight. At 0, there is no soft pressure and the generator may freely place beginners in Racing boats whenever the hard block is lifted in pass 2.",
              "Default: 8.0. Applied on top of the hard block in pass 0.",
              8.0, 0.0, 30.0, 0.5,
              [this](double v){ m_expert.racingBeginnerPenalty = v; m_db->saveExpertSetting("racingBeginnerPenalty", v); });
        vl->addWidget(g);
    }

    // ══ 5 — Strength Variance Balancing ═════════
    vl->addWidget(mkHeader("5 — Strength Variance Balancing"));
    {
        auto* g = new QGroupBox;
        auto* gl = new QVBoxLayout(g);
        mkDbl(w, gl, "strengthVarianceWeight  —  Penalty weight for unequal physical strength in a team",
              "score −= strengthVarianceWeight × variance(strength_values_in_team)   [cap > 2 only]",
              "strengthVarianceWeight scales how much the generator penalises uneven physical strength within a boat. 'strength' is the per-rower numeric field (0=not set, 1–10 set in Rowers tab). Variance = Σ(sᵢ − s̄)² / N where only rowers with strength > 0 are included. Only applies to boats with capacity > 2.\n"
              "Typical variance for a team of 4 with mixed strength is 2–8.",
              "Increase when boats often have one very strong and one very weak rower seated together. Useful when strength data is reliably entered for all rowers. Decrease (or set to 0) if strength data is incomplete or you trust the coach to assign seating positions manually.",
              "The generator more aggressively avoids teams with a wide strength spread. At 1.0 a variance of 6 costs 6.0 score — comparable to a whitelist bonus.",
              "Strength imbalance within a boat is tolerated more. Teams where one rower is much stronger than the others are not penalised. At 0, the strength field is completely ignored during generation.",
              "Default: 0.3. Typical effective penalty: 0.6–2.4 for a 4-rower team.",
              0.3, 0.0, 5.0, 0.1,
              [this](double v){ m_expert.strengthVarianceWeight = v; m_db->saveExpertSetting("strengthVarianceWeight", v); });
        vl->addWidget(g);
    }

    // ══ 6 — Compatibility Soft Penalties  (Special / Selected) ═════════
    vl->addWidget(mkHeader("6 — Compatibility Soft Penalties  (Special / Selected)"));
    {
        auto* g = new QGroupBox;
        auto* gl = new QVBoxLayout(g);
        gl->addWidget(new QLabel("<b>Context:</b> Compatibility tiers — Infinite, Normal, Special, Selected. Infinite/Normal impose no penalty. Special+Selected is also a HARD block in pass 0 (cannot be overridden here). These soft penalties are summed pairwise for every pair in the team, then multiplied by wCompat. A team of 4 has\n"
              "6 pairs; in a worst case all 6 pay the penalty."));
        gl->addSpacing(6);
        mkDbl(w, gl, "compatSpecialSpecial  —  Penalty for two 'Special' rowers in the same boat",
              "score −= wCompat × compatSpecialSpecial    (per Special+Special pair in team)",
              "compatSpecialSpecial is the base penalty added when two rowers both marked 'Special' end up in the same team. 'Special' means the rower has specific personal preferences and works best with certain people. The penalty is multiplied by wCompat, so the effective penalty also depends on how highly you\n"
              "ranked Compatibility in the priority list.",
              "Increase if your club has several 'Special' rowers who genuinely do not mix well in practice. Decrease if 'Special' is used loosely and most Special+Special combinations are fine.",
              "Special rowers are more aggressively separated across boats. At compatSpecialSpecial × wCompat > obmannBonus the separation can override Obmann placement.",
              "Special+Special pairs are tolerated more and the generator places them together more readily. At 0, two 'Special' rowers can share a boat with no score penalty — only the hard block for Special+Selected in pass 0 remains.",
              "Default: 2.0. Effective penalty = wCompat × 2.0 (typically 4.0–8.0).",
              2.0, 0.0, 10.0, 0.5,
              [this](double v){ m_expert.compatSpecialSpecial = v; m_db->saveExpertSetting("compatSpecialSpecial", v); });
        mkDbl(w, gl, "compatSpecialSelected  —  Penalty for a 'Special'+'Selected' pair in the same boat",
              "score −= wCompat × compatSpecialSelected    (per Special+Selected pair, passes 1–2 only)",
              "compatSpecialSelected is the base penalty for placing one 'Special' and one 'Selected' rower in the same team. In pass 0 this is a HARD block regardless of this value. In passes 1 and 2 (relaxed mode) the hard block is lifted and only this soft penalty discourages the pairing.",
              "Increase if you want to further discourage Special+Selected pairings in relaxed passes. Decrease if strict pass failures are common and you need the generator more flexibility.",
              "Special+Selected pairings become even more costly in passes 1–2. At compatSpecialSelected > 8 the generator will strongly prefer to fail gracefully rather than pair them.",
              "Special+Selected pairs in relaxed passes (1–2) cost less score. The generator is more willing to pair them when constraints are tight. Note: the hard block in pass 0 is unaffected by this value.",
              "Default: 4.0. Effective penalty = wCompat × 4.0 (typically 8.0–16.0).",
              4.0, 0.0, 15.0, 0.5,
              [this](double v){ m_expert.compatSpecialSelected = v; m_db->saveExpertSetting("compatSpecialSelected", v); });
        vl->addWidget(g);
    }

    // ══ 7 — Stroke Length Matching Penalties ═════════
    vl->addWidget(mkHeader("7 — Stroke Length Matching Penalties"));
    {
        auto* g = new QGroupBox;
        auto* gl = new QVBoxLayout(g);
        gl->addWidget(new QLabel("<b>Context:</b> Stroke length (Short=1, Medium=2, Long=3) is set per rower in the Rowers tab. The generator computes gap = |strokeLength_i − strokeLength_j| for every pair in the team. Two separate penalty scales apply: one for 2-seat boats (where mismatch is critical) and one for larger boats. Only\n"
              "rowers with a set value (>0) are included."));
        gl->addSpacing(6);
        mkDbl(w, gl, "strokeSmallGap1  —  Penalty for adjacent stroke lengths in a 2-seat boat",
              "score −= strokeSmallGap1    (per pair with |gap|=1 and boat cap ≤ 2)",
              "strokeSmallGap1 is the penalty for placing a Short+Medium or Medium+Long pair in a 2-seat boat. A gap of 1 means the rowers are one step apart on the Short/Medium/Long scale — adjacent but not identical.",
              "Increase if your 2-seat boats are very rhythm-sensitive (e.g. training sculls) and you want the generator to keep adjacent stroke lengths apart. Decrease if adjacent lengths are acceptable in 2-seat boats at your club.",
              "The generator becomes reluctant to put adjacent stroke lengths together in 2-seat boats. At strokeSmallGap1 > obmannBonus (20) it effectively becomes a hard block for 2-seat boats.",
              "Adjacent stroke lengths in 2-seat boats are tolerated with less penalty. The generator pairs Short+Medium or Medium+Long rowers more readily. At 0, a gap of 1 is completely free and only a gap of 2 (Short+Long) still costs strokeSmallGap2.",
              "Default: 3.0.",
              3.0, 0.0, 20.0, 0.5,
              [this](double v){ m_expert.strokeSmallGap1 = v; m_db->saveExpertSetting("strokeSmallGap1", v); });
        mkDbl(w, gl, "strokeSmallGap2  —  Penalty for maximum stroke length mismatch in a 2-seat boat",
              "score −= strokeSmallGap2    (per pair with |gap|=2 and boat cap ≤ 2)",
              "strokeSmallGap2 is the penalty for placing a Short+Long pair in a 2-seat boat — the worst possible mismatch. These two rowers have fundamentally different stroke mechanics and coordinated rowing becomes very difficult.",
              "Increase to make Short+Long pairings in 2-seat boats near-impossible. At values above 20 it matches the Obmann bonus and becomes dominant. Decrease if your 2-seat boats are used for mixed-technique training where mismatch is intended.",
              "Short+Long pairs become extremely unlikely in 2-seat boats. The generator will strongly prefer to separate them even at the cost of other factors.",
              "Short+Long pairings in 2-seat boats become more acceptable. The generator places them together when skill or compat considerations favour it. Useful if stroke length data is only loosely entered.",
              "Default: 12.0. Already strong — Short+Long in a 2-seat boat is treated as a near-block.",
              12.0, 0.0, 40.0, 1.0,
              [this](double v){ m_expert.strokeSmallGap2 = v; m_db->saveExpertSetting("strokeSmallGap2", v); });
        mkDbl(w, gl, "strokeLargePerGap  —  Penalty per stroke-length gap unit in larger boats",
              "score −= strokeLargePerGap × |gap|    (per pair, boat cap > 2)",
              "strokeLargePerGap scales the stroke length mismatch penalty for boats with more than 2 seats. Larger crews can absorb individual differences better so the penalty is lower and proportional to the gap. A gap of 1 costs strokeLargePerGap × 1, a gap of 2 costs strokeLargePerGap × 2.",
              "Increase if larger boats also need careful stroke length matching — e.g. for a competitive squad. Decrease (or set to 0) if stroke length should only matter for 2-seat boats.",
              "Stroke length differences become more costly in larger boats. At strokeLargePerGap = 5, a Short+Long pair in a 4-seat boat costs 10.0 — comparable to the Obmann bonus.",
              "Stroke length mismatches in larger boats cost less. The generator is more willing to mix Short, Medium and Long stroke rowers in 4+ seat boats. At 0, stroke length has no effect on placement in larger boats.",
              "Default: 2.5. Effective penalty per pair: 2.5 (gap=1) or 5.0 (gap=2).",
              2.5, 0.0, 10.0, 0.5,
              [this](double v){ m_expert.strokeLargePerGap = v; m_db->saveExpertSetting("strokeLargePerGap", v); });
        vl->addWidget(g);
    }

    // ══ 8 — Body Size Matching Penalties ═════════
    vl->addWidget(mkHeader("8 — Body Size Matching Penalties"));
    {
        auto* g = new QGroupBox;
        auto* gl = new QVBoxLayout(g);
        gl->addWidget(new QLabel("<b>Context:</b> Body size (Small=1, Medium=2, Tall=3) is set per rower in the Rowers tab. It follows the same pairwise gap logic as Stroke Length but with lower default penalties because body size affects boat balance less critically than stroke mechanics. The same two-scale system applies."));
        gl->addSpacing(6);
        mkDbl(w, gl, "bodySmallGap1  —  Penalty for adjacent body sizes in a 2-seat boat",
              "score −= bodySmallGap1    (per pair with |gap|=1 and boat cap ≤ 2)",
              "bodySmallGap1 is the penalty for placing Small+Medium or Medium+Tall rowers together in a 2-seat boat. One size step apart is generally acceptable in most boats but can create minor balance issues in very sensitive sculling pairs.",
              "Increase if your 2-seat boats are balance-sensitive (e.g. lightweight competition sculls). Decrease if body size is a minor consideration for 2-seat placements.",
              "The generator becomes more reluctant to put adjacent body sizes in 2-seat boats. Keep well below strokeSmallGap1 (default 3.0) since body size is less critical.",
              "Adjacent body sizes in 2-seat boats are accepted with less penalty. At 0, a one-step body size difference (Small+Medium or Medium+Tall) has no effect at all on 2-seat boat placement.",
              "Default: 1.5. Much lower than stroke length since adjacent sizes rarely cause problems.",
              1.5, 0.0, 15.0, 0.5,
              [this](double v){ m_expert.bodySmallGap1 = v; m_db->saveExpertSetting("bodySmallGap1", v); });
        mkDbl(w, gl, "bodySmallGap2  —  Penalty for maximum body size mismatch in a 2-seat boat",
              "score −= bodySmallGap2    (per pair with |gap|=2 and boat cap ≤ 2)",
              "bodySmallGap2 is the penalty for placing a Small+Tall pair in a 2-seat boat. This is the extreme case where a very small and very tall rower may create balance issues and find shared rhythm harder.",
              "Increase if your club has had practical problems with extreme size mismatches in pairs. Set to 20+ to treat it as a near-hard-block.",
              "Small+Tall pairings in 2-seat boats become increasingly unlikely. At very high values the generator accepts worse skill/compat matches to avoid the pairing.",
              "Small+Tall pairings in 2-seat boats are accepted more easily. The generator only avoids them when there is a clearly better alternative. At 0, extreme body size mismatches carry no penalty.",
              "Default: 8.0. Substantial but not dominant — allows the pairing when no better option exists.",
              8.0, 0.0, 30.0, 1.0,
              [this](double v){ m_expert.bodySmallGap2 = v; m_db->saveExpertSetting("bodySmallGap2", v); });
        mkDbl(w, gl, "bodyLargePerGap  —  Penalty per body size gap unit in larger boats",
              "score −= bodyLargePerGap × |gap|    (per pair, boat cap > 2)",
              "bodyLargePerGap scales the body size mismatch penalty for boats with more than 2 seats. Larger boats are much less sensitive to individual body size differences. The penalty is intentionally low.",
              "Increase if your club uses coxless fours or eights where body size uniformity is important for boat balance. Decrease or set to 0 if body size is irrelevant for larger boats.",
              "Body size differences in larger boats become slightly more costly. Rarely needs to exceed 2.0.",
              "Body size mismatches in larger boats are tolerated more freely. At 0, body size has no effect on placement in 4+ seat boats and only the 2-seat boat penalties remain.",
              "Default: 1.0. Very low — a Small+Tall pair in a 4-seat boat costs only 2.0.",
              1.0, 0.0, 8.0, 0.5,
              [this](double v){ m_expert.bodyLargePerGap = v; m_db->saveExpertSetting("bodyLargePerGap", v); });
        vl->addWidget(g);
    }

    // ══ 9 — Group Attributes & Value Balance Attributes ═════════
    vl->addWidget(mkHeader("9 — Group Attributes & Value Balance Attributes"));
    {
        auto* g = new QGroupBox;
        auto* gl = new QVBoxLayout(g);
        gl->addWidget(new QLabel("<b>Context:</b> The Rowers tab has four custom attributes: Grp Attr 1, Grp Attr 2 (group-together attributes) and Val Attr 1, Val Attr 2 (balance-across-boats attributes). All are numeric 0–10, where 0 means 'not set'. Group attributes reward placing rowers with the same value in the same boat.\n"
              "Value attributes reward having a similar average across all boats."));
        gl->addSpacing(6);
        mkDbl(w, gl, "grpAttrBonus  —  Bonus per matching pair in a Group Attribute",
              "score += grpAttrBonus    (per pair where GrpAttr1_i == GrpAttr1_j, and separately for GrpAttr2)",
              "grpAttrBonus is added to the team score for every pair of rowers who share the same non-zero value in GrpAttr1 (and independently in GrpAttr2). This encourages the generator to cluster rowers with the same group label together.",
              "Increase when group cohesion is important — e.g. training sessions where rowers from the same program should stay together. Decrease when mixing groups is desired. Set to 0 to ignore group attributes entirely.",
              "Rowers with matching group attributes are more strongly clustered. At grpAttrBonus > 10 the clustering can override skill balance.",
              "Group cohesion becomes a weaker preference. The generator mixes rowers from different groups more freely when skill or compat considerations favour it. At 0, group attributes are completely ignored during generation.",
              "Default: 3.0. A team of 4 with all same group value earns up to 6 × 3.0 = 18.0 bonus (6 pairs).",
              3.0, 0.0, 15.0, 0.5,
              [this](double v){ m_expert.grpAttrBonus = v; m_db->saveExpertSetting("grpAttrBonus", v); });
        mkDbl(w, gl, "valAttrVarianceWeight  —  Penalty weight for unequal value-attribute distribution across teams",
              "score −= valAttrVarianceWeight × variance(ValAttr1_in_team)    [cap > 2, applied per attribute]",
              "valAttrVarianceWeight scales how much the generator penalises uneven ValAttr values within a team. The mean squared deviation of ValAttr1 (and separately ValAttr2) across team members (with value > 0) is computed and multiplied by this weight.",
              "Increase when fair distribution of a measured quality across boats is important. Decrease (or set to 0) if ValAttr data is incomplete or balance is not a priority.",
              "Teams with extreme value spread are penalised more. At 1.0 a variance of 8 (very unequal) costs 8.0 — substantial compared to other factors.",
              "Uneven value distributions within a boat are tolerated more. The generator may concentrate rowers with high ValAttr in the same boat. At 0, the ValAttr fields have no effect on generation.",
              "Default: 0.4. Typical effective penalty: 0.8–3.2 for a 4-rower team.",
              0.4, 0.0, 5.0, 0.1,
              [this](double v){ m_expert.valAttrVarianceWeight = v; m_db->saveExpertSetting("valAttrVarianceWeight", v); });
        vl->addWidget(g);
    }

    // ══ 10 — Obmann & Steerer Role Selection Weights ═════════
    vl->addWidget(mkHeader("10 — Obmann & Steerer Role Selection Weights"));
    {
        auto* g = new QGroupBox;
        auto* gl = new QVBoxLayout(g);
        gl->addWidget(new QLabel("<b>Context:</b> After teams are placed, Obmann and Steerer roles are assigned per boat (only for boats with capacity > 2). Role selection uses separate scoring. Obmann: older rowers preferred (higher ageBand score), frequent recent Obmann duty penalised. Steerer: younger rowers preferred (lower\n"
              "ageBand score), frequent recent duty penalised. ageBand values: 20, 30, 40, 50, 60, 70, 80 (the lower decade bound)."));
        gl->addSpacing(6);
        mkDbl(w, gl, "obmannAgeWeight  —  How much older age is preferred for Obmann",
              "obmannScore += ageBand × obmannAgeWeight    (per Obmann-capable candidate)",
              "obmannAgeWeight controls how much the rower's age decade influences Obmann selection. ageBand is the lower bound of their decade (20/30/40/50/60/70/80). A rower aged 60–70 has ageBand=60; at obmannAgeWeight=0.5 they score +30 from age alone.",
              "Increase if you want to strongly prefer older, experienced members as Obmann. Decrease if the age preference should be a gentle nudge. Set to 0 if age should not influence Obmann selection at all (overuse penalty still applies).",
              "Older rowers gain a larger score advantage for the Obmann role. At 1.0, a 70-year-old scores +70 from age — decisively outweighing the overuse penalty.",
              "Age matters less for Obmann selection. Younger Obmann-capable rowers compete more equally with older ones. At 0, age has no influence and the role is decided purely by the overuse penalty — whoever was Obmann least recently.",
              "Default: 0.5. A 60-year-old scores +30; a 30-year-old scores +15.",
              0.5, 0.0, 3.0, 0.1,
              [this](double v){ m_expert.obmannAgeWeight = v; m_db->saveExpertSetting("obmannAgeWeight", v); });
        mkDbl(w, gl, "obmannOverusePenalty  —  Penalty for being Obmann too recently",
              "obmannScore −= recentObmann × obmannOverusePenalty    (recentObmann = sessions in last N)",
              "obmannOverusePenalty is subtracted from a rower's Obmann score for each time they served as Obmann in the most recent N sessions (N = overuseThreshold). recentObmann is loaded from the assignment_roles table.",
              "Increase when you want strict rotation — a rower who was Obmann twice recently should be skipped. Decrease if the age preference should dominate. Set to 0 to always pick the oldest qualified rower regardless of history.",
              "Rowers who were recently Obmann are penalised more heavily, rotating the role faster. At 5.0, two recent sessions cost −10.0 — enough to overcome an age advantage of 20 years.",
              "Recent Obmann duty is penalised less. The same qualified rower (typically the oldest) is more likely to be chosen repeatedly. At 0, there is no rotation and the same person is Obmann every session.",
              "Default: 3.0. One recent session = −3.0 penalty; two recent = −6.0.",
              3.0, 0.0, 10.0, 0.5,
              [this](double v){ m_expert.obmannOverusePenalty = v; m_db->saveExpertSetting("obmannOverusePenalty", v); });
        mkDbl(w, gl, "steerYouthWeight  —  How much younger age is preferred for Steerer",
              "steerScore += (100 − effectiveBand) × steerYouthWeight    (effectiveBand = ageBand or 50)",
              "steerYouthWeight controls how much the foot-steerer role favours younger rowers. The formula uses (100 − ageBand) so a younger rower scores higher. effectiveBand = ageBand if set, otherwise 50 for rowers without age data.",
              "Increase if foot-steering should strongly favour the youngest qualified rower. Decrease if age should be a minor factor. Set to 0 to ignore age for steerer selection (overuse penalty still applies).",
              "Younger qualified rowers gain a stronger advantage for the Steerer role. At 1.0, a 25-year-old scores +75 compared to a 55-year-old's +45 — a decisive gap.",
              "Age matters less for steerer selection. Older canSteer-capable rowers become more competitive. At 0, age has no influence and the steerer role is decided purely by the overuse penalty — whoever steered least recently.",
              "Default: 0.3. A 25-year-old scores +22.5; a 55-year-old scores +13.5.",
              0.3, 0.0, 3.0, 0.1,
              [this](double v){ m_expert.steerYouthWeight = v; m_db->saveExpertSetting("steerYouthWeight", v); });
        mkDbl(w, gl, "steerOverusePenalty  —  Penalty for being Steerer too recently",
              "steerScore −= recentSteering × steerOverusePenalty    (recentSteering = sessions in last N)",
              "steerOverusePenalty is subtracted from a rower's Steerer score for each time they served as foot-steerer in the most recent N sessions. Same mechanism as Obmann overuse — ensures the steering duty rotates.",
              "Increase for strict steering rotation. Decrease to allow the same person to steer repeatedly if they are clearly the best candidate.",
              "The steering role rotates faster across all qualified rowers.",
              "The same qualified rower steers repeatedly with less penalty. At 0, there is no rotation at all and the youngest canSteer-capable rower will steer every session.",
              "Default: 3.0. Same as obmannOverusePenalty for symmetric rotation.",
              3.0, 0.0, 10.0, 0.5,
              [this](double v){ m_expert.steerOverusePenalty = v; m_db->saveExpertSetting("steerOverusePenalty", v); });
        vl->addWidget(g);
    }

    // ══ 11 — Generator Search Depth ═════════
    vl->addWidget(mkHeader("11 — Generator Search Depth"));
    {
        auto* g = new QGroupBox;
        auto* gl = new QVBoxLayout(g);
        gl->addWidget(new QLabel("<b>Context:</b> The generator is stochastic. It randomly shuffles rowers and greedily builds candidate teams, then keeps the best-scoring valid team across many attempts. More attempts means a better chance of finding a high-quality solution, but also means longer generation time. The algorithm runs\n"
              "up to 3 passes, and within each pass runs up to passAttempts full-assignment attempts."));
        gl->addSpacing(6);
        vl->addWidget(g);
    }

    // ══ 11 — Generator Search Depth (int settings) ═════════
    vl->addWidget(mkHeader("11 — Generator Search Depth"));
    {
        auto* g = new QGroupBox;
        auto* gl = new QVBoxLayout(g);
        gl->addWidget(new QLabel("<b>Context:</b> The generator is stochastic. It randomly shuffles rowers and greedily builds candidate teams, keeping the best-scoring valid team across many attempts. More attempts means a better chance of finding a high-quality solution, but also means longer generation time. The algorithm runs up\n"
              "to 3 passes, each with up to passAttempts full-assignment attempts."));
        gl->addSpacing(6);
        mkInt(w, gl, "overuseThreshold  —  How many recent sessions define 'overused'",
              "overused flag shown when recentCount ≥ overuseThreshold    (in the Statistics tab)",
              "overuseThreshold defines the window size N used for overuse tracking. recentObmann and recentSteering count how many times a rower held that role in the last overuseThreshold assignments. When the count reaches overuseThreshold, the Statistics tab flags the rower as 'overused'.",
              "Increase if your club has sessions frequently (e.g. 3+ per week) and you want overuse to be measured over a longer period. Decrease for clubs with infrequent sessions where even 2 consecutive appearances should trigger rotation.",
              "The overuse flag appears less frequently. The penalty window is wider, so rotation is spread over more sessions.",
              "The overuse flag appears sooner — fewer sessions of the same role trigger it. Role rotation becomes more aggressive. At 1, a rower who served even once in the last session is immediately flagged and penalised.",
              "Default: 3. A rower who was Obmann in 3 of the last 3 sessions is flagged.",
              3, 1, 20,
              [this](int v){ m_expert.overuseThreshold = v; m_db->saveExpertSetting("overuseThreshold", static_cast<double>(v)); });
        mkInt(w, gl, "fillBoatAttempts  —  Random shuffle attempts per boat per pass",
              "for attempt in 0 … fillBoatAttempts:  shuffle → build team → score → keep best",
              "fillBoatAttempts is the number of random team compositions tried for each individual boat within a single full-assignment attempt. The highest-scoring valid team across all attempts is committed for that boat.",
              "Increase when the generator produces mediocre results in complex scenarios with many constraints or attributes. Rarely useful to exceed 1000 — improvement diminishes above 600.",
              "Each additional attempt improves solution quality logarithmically. Generation time scales linearly with this value.",
              "Fewer random compositions are tried per boat. Generation is faster but quality may be lower, especially in complex scenarios. At 10 (minimum) the generator has very little chance of finding an optimal team.",
              "Default: 600. Halving to 300 is barely noticeable; below 50 quality may degrade.",
              600, 10, 5000,
              [this](int v){ m_expert.fillBoatAttempts = v; m_db->saveExpertSetting("fillBoatAttempts", static_cast<double>(v)); });
        mkInt(w, gl, "passAttempts  —  Full-assignment attempts per generation pass",
              "for fa in 0 … passAttempts:  fill all boats in sequence → keep first fully successful attempt",
              "passAttempts is the number of times the generator tries to fill ALL boats in one full assignment attempt within a single pass. The loop only matters when early boats use up rowers that later boats need — a second attempt shuffles differently and may succeed.",
              "Increase if you frequently see 'could not generate a valid assignment' errors even though Check shows no hard problems — this suggests rower allocation order is causing some boats to fail.",
              "The generator gets more chances to find a feasible full assignment, reducing errors at the cost of slightly longer generation time.",
              "Fewer full-assignment retries are allowed. At 1, there is only one attempt per pass — if any boat cannot be filled due to allocation order, the pass fails immediately.",
              "Default: 15. Most scenarios succeed in 1–3 attempts; rarely useful above 30.",
              15, 1, 100,
              [this](int v){ m_expert.passAttempts = v; m_db->saveExpertSetting("passAttempts", static_cast<double>(v)); });
        vl->addWidget(g);
    }


    // ── Change Password ───────────────────────────────────────────
    vl->addSpacing(10);
    vl->addWidget(mkHeader("12 — Change Password"));
    {
        auto* g = new QGroupBox;
        auto* gl = new QVBoxLayout(g);
        gl->addWidget(new QLabel(
            "<b>Change the password required to unlock and edit Expert Settings and to unlock assignments.</b>\n"
            "Default factory password is: 0815"));
        gl->addSpacing(6);

        auto* oldPwEdit = new QLineEdit;
        oldPwEdit->setEchoMode(QLineEdit::Password);
        oldPwEdit->setPlaceholderText("Current password");
        oldPwEdit->setEnabled(false);
        spinboxes->append(oldPwEdit);

        auto* newPwEdit = new QLineEdit;
        newPwEdit->setEchoMode(QLineEdit::Password);
        newPwEdit->setPlaceholderText("New password");
        newPwEdit->setEnabled(false);
        spinboxes->append(newPwEdit);

        auto* confirmPwEdit = new QLineEdit;
        confirmPwEdit->setEchoMode(QLineEdit::Password);
        confirmPwEdit->setPlaceholderText("Repeat new password");
        confirmPwEdit->setEnabled(false);
        spinboxes->append(confirmPwEdit);

        auto* changePwBtn = new QPushButton("Change Password");
        changePwBtn->setObjectName("primaryBtn");
        changePwBtn->setEnabled(false);
        spinboxes->append(changePwBtn);

        gl->addWidget(oldPwEdit);
        gl->addWidget(newPwEdit);
        gl->addWidget(confirmPwEdit);
        gl->addWidget(changePwBtn, 0, Qt::AlignLeft);

        QObject::connect(changePwBtn, &QPushButton::clicked, w,
            [this, oldPwEdit, newPwEdit, confirmPwEdit, changePwBtn, unlocked]() {
                if (!*unlocked) return;
                QString oldPw = oldPwEdit->text();
                QString newPw = newPwEdit->text();
                QString confirm = confirmPwEdit->text();
                if (oldPw != m_password) {
                    QMessageBox::warning(nullptr, "Wrong Password", "Current password is incorrect.");
                    return;
                }
                if (newPw.isEmpty()) {
                    QMessageBox::warning(nullptr, "Empty Password", "New password cannot be empty.");
                    return;
                }
                if (newPw != confirm) {
                    QMessageBox::warning(nullptr, "Mismatch",
                        "New password and confirmation do not match.");
                    return;
                }
                m_password = newPw;
                m_db->savePassword(newPw);
                oldPwEdit->clear();
                newPwEdit->clear();
                confirmPwEdit->clear();
                QMessageBox::information(nullptr, "Password Changed",
                    "Password changed successfully.");
            });

        vl->addWidget(g);
    }

    // ── Reset button ─────────────────────────────────────────────
    vl->addSpacing(10);
    auto* resetBtn = new QPushButton("↺  Reset all settings to factory defaults");
    resetBtn->setObjectName("dangerBtn");
    resetBtn->setEnabled(false);
    spinboxes->append(resetBtn);
    resetBtn->setToolTip("Restores all 27 settings to their original coded values and saves them to the database.");
    vl->addWidget(resetBtn, 0, Qt::AlignLeft);
    QObject::connect(resetBtn, &QPushButton::clicked, w, [this, outer]() {
        m_expert = ExpertSettings{};
        auto s = [this](const QString& k, double v){ m_db->saveExpertSetting(k, v); };
        s("weightRank1",4.0); s("weightRank2",2.0); s("weightRank3",1.0);
        s("weightRank4",0.5); s("weightRank5",0.5);
        s("whitelistBonus",5.0); s("coOccurrenceFactor",1.5);
        s("obmannBonus",20.0); s("racingBeginnerPenalty",8.0);
        s("strengthVarianceWeight",0.3);
        s("compatSpecialSpecial",2.0); s("compatSpecialSelected",4.0);
        s("strokeSmallGap1",3.0); s("strokeSmallGap2",12.0); s("strokeLargePerGap",2.5);
        s("bodySmallGap1",1.5); s("bodySmallGap2",8.0); s("bodyLargePerGap",1.0);
        s("grpAttrBonus",3.0); s("valAttrVarianceWeight",0.4);
        s("obmannAgeWeight",0.5); s("obmannOverusePenalty",3.0);
        s("steerYouthWeight",0.3); s("steerOverusePenalty",3.0);
        s("overuseThreshold",3.0); s("fillBoatAttempts",600.0); s("passAttempts",15.0);
        // Rebuild tab
        for (int i = 0; i < m_tabs->count(); ++i)
            if (m_tabs->tabText(i) == "Expert Settings") {
                m_tabs->removeTab(i);
                m_tabs->insertTab(i, buildExpertTab(), "Expert Settings");
                m_tabs->setCurrentIndex(i);
                break;
            }
        statusBar()->showMessage("All 27 expert settings reset to factory defaults and saved.", 3000);
    });

    vl->addStretch();
    scroll->setWidget(w);
    outerVL->addWidget(scroll);
    return outer;
}


// ---------------------------------------------------------------
// Expert settings persistence
// ---------------------------------------------------------------
void MainWindow::loadExpertSettings() {
    // Load password
    m_password = m_db->loadPassword();

    QMap<QString,double> s = m_db->loadExpertSettings();
    if (s.isEmpty()) return;   // no saved settings → keep code defaults

    auto get = [&](const QString& k, double def) { return s.value(k, def); };

    m_expert.weightRank1            = get("weightRank1",            4.0);
    m_expert.weightRank2            = get("weightRank2",            2.0);
    m_expert.weightRank3            = get("weightRank3",            1.0);
    m_expert.weightRank4            = get("weightRank4",            0.5);
    m_expert.weightRank5            = get("weightRank5",            0.5);
    m_expert.whitelistBonus         = get("whitelistBonus",         5.0);
    m_expert.coOccurrenceFactor     = get("coOccurrenceFactor",     1.5);
    m_expert.obmannBonus            = get("obmannBonus",            20.0);
    m_expert.racingBeginnerPenalty  = get("racingBeginnerPenalty",  8.0);
    m_expert.strengthVarianceWeight = get("strengthVarianceWeight", 0.3);
    m_expert.compatSpecialSpecial   = get("compatSpecialSpecial",   2.0);
    m_expert.compatSpecialSelected  = get("compatSpecialSelected",  4.0);
    m_expert.strokeSmallGap1        = get("strokeSmallGap1",        3.0);
    m_expert.strokeSmallGap2        = get("strokeSmallGap2",        12.0);
    m_expert.strokeLargePerGap      = get("strokeLargePerGap",      2.5);
    m_expert.bodySmallGap1          = get("bodySmallGap1",          1.5);
    m_expert.bodySmallGap2          = get("bodySmallGap2",          8.0);
    m_expert.bodyLargePerGap        = get("bodyLargePerGap",        1.0);
    m_expert.grpAttrBonus           = get("grpAttrBonus",           3.0);
    m_expert.valAttrVarianceWeight  = get("valAttrVarianceWeight",  0.4);
    m_expert.obmannAgeWeight        = get("obmannAgeWeight",        0.5);
    m_expert.obmannOverusePenalty   = get("obmannOverusePenalty",   3.0);
    m_expert.steerYouthWeight       = get("steerYouthWeight",       0.3);
    m_expert.steerOverusePenalty    = get("steerOverusePenalty",    3.0);
    m_expert.overuseThreshold       = static_cast<int>(get("overuseThreshold", 3.0));
    m_expert.fillBoatAttempts       = static_cast<int>(get("fillBoatAttempts", 600.0));
    m_expert.passAttempts           = static_cast<int>(get("passAttempts",     15.0));
}

// ---------------------------------------------------------------
// Password check helper
// ---------------------------------------------------------------
bool MainWindow::checkPassword(const QString& action) {
    QDialog dlg(this);
    dlg.setWindowTitle("Password Required");
    dlg.setModal(true);
    auto* vl = new QVBoxLayout(&dlg);

    auto* lbl = new QLabel(QString("Enter password to %1:").arg(action));
    lbl->setStyleSheet("color:#c8d8e8; font-size:12px;");
    vl->addWidget(lbl);

    auto* pwEdit = new QLineEdit;
    pwEdit->setEchoMode(QLineEdit::Password);
    pwEdit->setPlaceholderText("Password");
    vl->addWidget(pwEdit);

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    vl->addWidget(btns);
    QObject::connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    QObject::connect(pwEdit, &QLineEdit::returnPressed, &dlg, &QDialog::accept);

    if (dlg.exec() != QDialog::Accepted) return false;
    if (pwEdit->text() == m_password) return true;
    QMessageBox::warning(this, "Incorrect Password", "The password you entered is incorrect.");
    return false;
}

// =========================================================================
// Analysis Tab — rich statistics, tables, and charts across all assignments
// =========================================================================
QWidget* MainWindow::buildAnalysisTab() {
    auto* outer = new QWidget;
    auto* outerVL = new QVBoxLayout(outer);
    outerVL->setContentsMargins(0,0,0,0);

    auto* toolbar = new QHBoxLayout;
    auto* titleLbl = new QLabel("<b style='color:#8fb4d8; font-size:13px;'>Analysis Dashboard</b>"
        "  <span style='color:#5a7a9a; font-size:11px;'>"
        "— all scoring insights across rowers, boats, and assignments</span>");
    auto* refreshBtn = new QPushButton("⟳  Refresh");
    refreshBtn->setObjectName("primaryBtn");
    toolbar->addWidget(titleLbl, 1);
    toolbar->addWidget(refreshBtn);
    outerVL->addLayout(toolbar);

    // Two sub-tabs: Table View + Graphical View
    auto* subTabs = new QTabWidget;
    outerVL->addWidget(subTabs, 1);

    // ── Build the inner scroll area for table view ──────────────
    auto* tableScrollOuter = new QScrollArea;
    tableScrollOuter->setWidgetResizable(true);
    tableScrollOuter->setFrameShape(QFrame::NoFrame);
    m_analysisInner = new QWidget;
    tableScrollOuter->setWidget(m_analysisInner);
    subTabs->addTab(tableScrollOuter, "Table View");

    // ── Graphical sub-tab ────────────────────────────────────────
    auto* gfxScrollOuter = new QScrollArea;
    gfxScrollOuter->setWidgetResizable(true);
    gfxScrollOuter->setFrameShape(QFrame::NoFrame);
    auto* gfxInner = new QWidget;
    auto* gfxVL = new QVBoxLayout(gfxInner);
    gfxVL->setContentsMargins(8,8,8,16);
    gfxVL->setSpacing(16);
    gfxScrollOuter->setWidget(gfxInner);
    subTabs->addTab(gfxScrollOuter, "Graphical View");

    // Connect refresh
    QObject::connect(refreshBtn, &QPushButton::clicked, this, [this, gfxInner, gfxVL]() {
        refreshAnalysisTab();
        // Rebuild graphical view
        while (QLayoutItem* it = gfxVL->takeAt(0)) { delete it->widget(); delete it; }
        buildAnalysisGraphics(gfxVL);
        gfxVL->addStretch();
    });

    // Initial populate
    refreshAnalysisTab();
    buildAnalysisGraphics(gfxVL);
    gfxVL->addStretch();

    return outer;
}

// Called to rebuild the table-view portion of the Analysis tab
void MainWindow::refreshAnalysisTab() {
    if (!m_analysisInner) return;

    // Clear existing layout
    if (auto* old = m_analysisInner->layout()) {
        while (QLayoutItem* it = old->takeAt(0)) { delete it->widget(); delete it; }
        delete old;
    }

    auto* vl = new QVBoxLayout(m_analysisInner);
    vl->setContentsMargins(8,8,8,16);
    vl->setSpacing(16);

    auto mkSec = [&](const QString& t, const QString& desc="") {
        auto* l = new QLabel(t);
        l->setStyleSheet("color:#8fb4d8; font-weight:700; font-size:12px; "
                         "border-bottom:1px solid #2a3548; padding-bottom:3px; margin-top:4px;");
        vl->addWidget(l);
        if (!desc.isEmpty()) {
            auto* d = new QLabel(desc);
            d->setWordWrap(true);
            d->setStyleSheet("color:#5a7a9a; font-size:11px; padding-left:4px;");
            vl->addWidget(d);
        }
    };

    auto mkTable = [&](const QStringList& headers) -> QTableWidget* {
        auto* t = new QTableWidget(0, headers.size());
        t->setHorizontalHeaderLabels(headers);
        t->setEditTriggers(QAbstractItemView::NoEditTriggers);
        t->setSelectionMode(QAbstractItemView::NoSelection);
        t->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        t->verticalHeader()->setVisible(false);
        t->setAlternatingRowColors(true);
        t->setStyleSheet(
            "QTableWidget{background:#0a1520;alternate-background-color:#0d1e2e;gridline-color:#1a2538;}"
            "QHeaderView::section{background:#1a2535;color:#8fb4d8;font-weight:600;padding:4px;border:1px solid #2a3548;}"
            "QTableWidget::item{padding:3px 6px;}");
        return t;
    };

    auto addCell = [](QTableWidget* t, int row, int col, const QString& val,
                      const QColor& fg=QColor(), bool bold=false) {
        auto* item = new QTableWidgetItem(val);
        if (fg.isValid()) item->setForeground(fg);
        if (bold) { QFont f=item->font(); f.setBold(true); item->setFont(f); }
        t->setItem(row, col, item);
    };

    // ────────────────────────────────────────────────────────────
    // TABLE 1: Rower overview — all attributes side by side
    // ────────────────────────────────────────────────────────────
    mkSec("Rower Attribute Overview",
          "All rower attributes in one place. Use this to spot missing data (—) "
          "and to check that attribute ranges make sense before generating assignments.");
    {
        auto* t = mkTable({"Name","Skill","Compat","Prop","Strength","Stroke","Body",
                           "GrpA1","GrpA2","ValA1","ValA2","Obmann","Steer",
                           "RW","RB","BW","BB"});
        const QString SL[]={"—","Short","Med","Long"}, BS[]={"—","Small","Med","Tall"};
        for (const Rower& r : m_rowers) {
            int row = t->rowCount(); t->insertRow(row);
            addCell(t,row,0,r.name(),{},true);
            addCell(t,row,1,Rower::skillToString(r.skill()));
            addCell(t,row,2,Rower::compatToString(r.compatibility()));
            addCell(t,row,3,Boat::propulsionTypeToString(r.propulsionAbility()));
            addCell(t,row,4,r.strength()>0?QString::number(r.strength()):"—");
            addCell(t,row,5,r.strokeLength()>=0&&r.strokeLength()<=3?SL[r.strokeLength()]:"—");
            addCell(t,row,6,r.bodySize()>=0&&r.bodySize()<=3?BS[r.bodySize()]:"—");
            addCell(t,row,7,r.attrGrp1()>0?QString::number(r.attrGrp1()):"—");
            addCell(t,row,8,r.attrGrp2()>0?QString::number(r.attrGrp2()):"—");
            addCell(t,row,9,r.attrVal1()>0?QString::number(r.attrVal1()):"—");
            addCell(t,row,10,r.attrVal2()>0?QString::number(r.attrVal2()):"—");
            addCell(t,row,11,r.isObmann()?"✓":"",r.isObmann()?QColor("#88ee88"):QColor());
            addCell(t,row,12,r.canSteer()?"✓":"",r.canSteer()?QColor("#88ccff"):QColor());
            addCell(t,row,13,QString::number(r.whitelist().size()));
            addCell(t,row,14,QString::number(r.blacklist().size()),
                    r.blacklist().isEmpty()?QColor():QColor("#ee8888"));
            addCell(t,row,15,QString::number(r.boatWhitelist().size()));
            addCell(t,row,16,QString::number(r.boatBlacklist().size()),
                    r.boatBlacklist().isEmpty()?QColor():QColor("#ee8888"));
        }
        t->resizeRowsToContents();
        vl->addWidget(t);
    }

    // ────────────────────────────────────────────────────────────
    // TABLE 2: Attribute completeness / data quality
    // ────────────────────────────────────────────────────────────
    mkSec("Data Quality — Attribute Completeness",
          "Shows what percentage of rowers have each attribute set. "
          "Attributes with low fill rates will have little effect on generation since "
          "values of 0 are excluded from all scoring calculations.");
    {
        auto* t = mkTable({"Attribute","Set","Not Set","Fill %","Recommendation"});
        struct AttrCheck { QString name; std::function<bool(const Rower&)> fn; QString tip; };
        QList<AttrCheck> checks = {
            {"Strength",    [](const Rower& r){return r.strength()>0;},
             "Used to balance physical load. Enter for all rowers."},
            {"Stroke Length",[](const Rower& r){return r.strokeLength()>0;},
             "Critical for 2-seat boats. Enter for all rowers."},
            {"Body Size",   [](const Rower& r){return r.bodySize()>0;},
             "Used for 2-seat boat balance. Recommended."},
            {"Age Band",    [](const Rower& r){return r.ageBand()>0;},
             "Used for Obmann/Steerer role selection."},
            {"GrpAttr 1",   [](const Rower& r){return r.attrGrp1()>0;},
             "Only useful if you have a club grouping to model."},
            {"GrpAttr 2",   [](const Rower& r){return r.attrGrp2()>0;},
             "Only useful if you have a second club grouping."},
            {"ValAttr 1",   [](const Rower& r){return r.attrVal1()>0;},
             "Technique/fitness score. Enter consistently or leave blank."},
            {"ValAttr 2",   [](const Rower& r){return r.attrVal2()>0;},
             "Second balance attribute. Enter consistently or leave blank."},
        };
        int total = m_rowers.size();
        for (auto& ck : checks) {
            int cnt=0; for (const Rower& r:m_rowers) if(ck.fn(r)) cnt++;
            int row=t->rowCount(); t->insertRow(row);
            double pct = total>0 ? 100.0*cnt/total : 0;
            addCell(t,row,0,ck.name,{},true);
            addCell(t,row,1,QString::number(cnt));
            addCell(t,row,2,QString::number(total-cnt),
                    (total-cnt)>0?QColor("#ee9966"):QColor("#88ee88"));
            addCell(t,row,3,QString::number(pct,'f',0)+"%",
                    pct<50?QColor("#ee6644"):pct<80?QColor("#eecc44"):QColor("#88ee88"));
            addCell(t,row,4,ck.tip);
        }
        t->resizeRowsToContents();
        vl->addWidget(t);
    }

    // ────────────────────────────────────────────────────────────
    // TABLE 3: Scoring breakdown across all locked assignments
    // ────────────────────────────────────────────────────────────
    mkSec("Scoring History — All Locked Assignments",
          "For each locked assignment, shows the average total score and the "
          "average per-penalty values across all boats. "
          "Use this to track improvement over time as you tune Expert Settings.");
    {
        auto* t = mkTable({"Assignment","Date","Boats","AvgScore","AvgSkill",
                           "AvgCompat","AvgStroke","AvgBody","AvgCoOcc","AvgStrength"});
        ScoringPriority defPriority;
        AssignmentGenerator gen;
        for (const Assignment& a : m_assignments) {
            if (!a.isLocked()) continue;
            Assignment full = m_db->loadAssignment(a.id());
            QList<ScoreDetail> details = gen.computeScoreDetails(full, m_boats, m_rowers, defPriority);
            if (details.isEmpty()) continue;
            double avgScore=0,avgSkill=0,avgComp=0,avgStr=0,avgBody=0,avgCo=0,avgStrV=0;
            for (auto& d:details) {
                avgScore+=d.totalScore; avgSkill+=d.avgSkill;
                avgComp+=d.compatPenalty; avgStr+=d.strokePenalty;
                avgBody+=d.bodyPenalty; avgCo+=d.coOccurrencePenalty;
                avgStrV+=d.strengthVariance;
            }
            double n=details.size();
            int row=t->rowCount(); t->insertRow(row);
            addCell(t,row,0,a.name(),{},true);
            addCell(t,row,1,a.createdAt().toString("dd.MM.yyyy"));
            addCell(t,row,2,QString::number((int)n));
            addCell(t,row,3,QString::number(avgScore/n,'f',1),
                    avgScore/n>=0?QColor("#88ee88"):QColor("#ee6644"));
            addCell(t,row,4,QString::number(avgSkill/n,'f',2));
            addCell(t,row,5,QString::number(avgComp/n,'f',2),
                    avgComp/n>2?QColor("#ee9944"):QColor());
            addCell(t,row,6,QString::number(avgStr/n,'f',2),
                    avgStr/n>3?QColor("#ee9944"):QColor());
            addCell(t,row,7,QString::number(avgBody/n,'f',2));
            addCell(t,row,8,QString::number(avgCo/n,'f',2),
                    avgCo/n>3?QColor("#ee9944"):QColor());
            addCell(t,row,9,QString::number(avgStrV/n,'f',2));
        }
        if (t->rowCount()==0) {
            t->insertRow(0);
            addCell(t,0,0,"No locked assignments yet — lock an assignment to see history.",
                    QColor("#5a7a9a"));
        }
        t->resizeRowsToContents();
        vl->addWidget(t);
    }

    // ────────────────────────────────────────────────────────────
    // TABLE 4: Per-rower scoring contribution across locked assignments
    // ────────────────────────────────────────────────────────────
    mkSec("Rower Scoring Contribution — Latest Locked Assignment",
          "Shows each rower's individual attribute values alongside the scoring "
          "contributions they made in the most recent locked assignment. "
          "High coOccurrence penalty = rower is repeatedly paired with the same people.");
    {
        auto* t = mkTable({"Rower","Boat","Skill","Prop","Compat","Strength",
                           "StrokeLen","BodySize","GrpA1","GrpA2","ValA1","ValA2",
                           "WL Bonus","CoOcc Pen","Obmann","CanSteer"});

        // Find most recent locked assignment
        Assignment latest;
        for (const Assignment& a : m_assignments)
            if (a.isLocked()) { latest = m_db->loadAssignment(a.id()); break; }

        if (!latest.boatRowerMap().isEmpty()) {
            ScoringPriority defPriority;
            AssignmentGenerator gen;
            QList<ScoreDetail> details = gen.computeScoreDetails(latest, m_boats, m_rowers, defPriority);
            const QString SL[]={"—","Short","Med","Long"}, BS[]={"—","Small","Med","Tall"};
            for (const ScoreDetail& d : details) {
                QString bname = QString("Boat#%1").arg(d.boatId);
                for (const Boat& b:m_boats) if(b.id()==d.boatId){bname=b.name();break;}
                for (const ScoreDetail::RowerDetail& r : d.rowers) {
                    QString rname=QString("Rower#%1").arg(r.rowerId);
                    for (const Rower& ro:m_rowers) if(ro.id()==r.rowerId){rname=ro.name();break;}
                    int row=t->rowCount(); t->insertRow(row);
                    addCell(t,row,0,rname,{},true);
                    addCell(t,row,1,bname);
                    addCell(t,row,2,QString::number(r.skillInt));
                    addCell(t,row,3,r.propScore==1.0?"Exact":r.propScore==0.5?"Both":"None",
                            r.propScore==0?"#ee6644":QColor());
                    addCell(t,row,4,r.compatTier,
                            (r.compatTier=="Special"||r.compatTier=="Selected")?QColor("#eecc44"):QColor());
                    addCell(t,row,5,r.strength>0?QString::number(r.strength):"—");
                    addCell(t,row,6,r.strokeLength>=0&&r.strokeLength<=3?SL[r.strokeLength]:"—");
                    addCell(t,row,7,r.bodySize>=0&&r.bodySize<=3?BS[r.bodySize]:"—");
                    addCell(t,row,8,r.attrGrp1>0?QString::number(r.attrGrp1):"—");
                    addCell(t,row,9,r.attrGrp2>0?QString::number(r.attrGrp2):"—");
                    addCell(t,row,10,r.attrVal1>0?QString::number(r.attrVal1):"—");
                    addCell(t,row,11,r.attrVal2>0?QString::number(r.attrVal2):"—");
                    addCell(t,row,12,r.whitelistContrib>0?
                            QString("+%1").arg(r.whitelistContrib,0,'f',1):"0",
                            r.whitelistContrib>0?QColor("#88ee88"):QColor());
                    addCell(t,row,13,r.coOccContrib>0?
                            QString("−%1").arg(r.coOccContrib,0,'f',1):"0",
                            r.coOccContrib>3?QColor("#ee6644"):r.coOccContrib>0?QColor("#ee9944"):QColor());
                    addCell(t,row,14,r.isObmann?"✓":"",r.isObmann?QColor("#88ee88"):QColor());
                    addCell(t,row,15,r.canSteer?"✓":"",r.canSteer?QColor("#88ccff"):QColor());
                }
            }
        } else {
            t->insertRow(0);
            addCell(t,0,0,"No locked assignments found.", QColor("#5a7a9a"));
        }
        t->resizeRowsToContents();
        vl->addWidget(t);
    }

    // ────────────────────────────────────────────────────────────
    // TABLE 5: Blacklist & Whitelist network
    // ────────────────────────────────────────────────────────────
    mkSec("List Network — Whitelist & Blacklist Connections",
          "Each row is a rower with at least one list entry. "
          "Shows who they want to row with (whitelist) and who they must be separated from (blacklist). "
          "Large blacklist counts may indicate social friction and can cause generation failures.");
    {
        auto* t = mkTable({"Rower","Rower Whitelist (wants together)",
                           "Rower Blacklist (must separate)","Boat Whitelist","Boat Blacklist"});
        auto nameOf = [this](int id) {
            for (const Rower& r:m_rowers) if(r.id()==id) return r.name();
            return QString("Rower#%1").arg(id);
        };
        auto boatNameOf = [this](int id) {
            for (const Boat& b:m_boats) if(b.id()==id) return b.name();
            return QString("Boat#%1").arg(id);
        };
        for (const Rower& r : m_rowers) {
            bool hasAny = !r.whitelist().isEmpty() || !r.blacklist().isEmpty()
                       || !r.boatWhitelist().isEmpty() || !r.boatBlacklist().isEmpty();
            if (!hasAny) continue;
            int row=t->rowCount(); t->insertRow(row);
            addCell(t,row,0,r.name(),{},true);
            QStringList wln; for(int id:r.whitelist()) wln<<nameOf(id);
            QStringList bln; for(int id:r.blacklist()) bln<<nameOf(id);
            QStringList bwln; for(int id:r.boatWhitelist()) bwln<<boatNameOf(id);
            QStringList bbln; for(int id:r.boatBlacklist()) bbln<<boatNameOf(id);
            addCell(t,row,1,wln.isEmpty()?"(none)":wln.join(", "),
                    wln.isEmpty()?QColor("#5a7a9a"):QColor("#88ee88"));
            addCell(t,row,2,bln.isEmpty()?"(none)":bln.join(", "),
                    bln.isEmpty()?QColor("#5a7a9a"):QColor("#ee8866"));
            addCell(t,row,3,bwln.isEmpty()?"(none)":bwln.join(", "),
                    bwln.isEmpty()?QColor("#5a7a9a"):QColor("#88ccff"));
            addCell(t,row,4,bbln.isEmpty()?"(none)":bbln.join(", "),
                    bbln.isEmpty()?QColor("#5a7a9a"):QColor("#ee8866"));
        }
        if (t->rowCount()==0) {
            t->insertRow(0);
            addCell(t,0,0,"No list entries found — all rowers have empty lists.",QColor("#5a7a9a"));
        }
        t->resizeRowsToContents();
        vl->addWidget(t);
    }

    // ────────────────────────────────────────────────────────────
    // TABLE 6: Co-occurrence frequency
    // ────────────────────────────────────────────────────────────
    mkSec("Co-occurrence Frequency — Most Repeated Pairings",
          "Pairs of rowers who have shared a boat the most times. "
          "High counts may cause score penalties if coOccurrenceFactor > 0. "
          "Consider adding Blacklist entries for very high counts to force variety, "
          "or increase coOccurrenceFactor in Expert Settings.");
    {
        auto* t = mkTable({"Rower A","Rower B","Times Together","Penalty (factor=1.5)","Action"});
        auto co = m_db->loadCoOccurrence();
        // Sort by count descending
        QList<QPair<QPair<int,int>,int>> sorted;
        for (auto it=co.constBegin(); it!=co.constEnd(); ++it)
            sorted << qMakePair(it.key(), it.value());
        std::sort(sorted.begin(), sorted.end(),
                  [](const auto& a, const auto& b){ return a.second > b.second; });
        // Show top 30
        int shown=0;
        for (auto& kv : sorted) {
            if (++shown > 30) break;
            QString nameA, nameB;
            for (const Rower& r:m_rowers) {
                if(r.id()==kv.first.first) nameA=r.name();
                if(r.id()==kv.first.second) nameB=r.name();
            }
            if (nameA.isEmpty()) nameA=QString("Rower#%1").arg(kv.first.first);
            if (nameB.isEmpty()) nameB=QString("Rower#%1").arg(kv.first.second);
            int row=t->rowCount(); t->insertRow(row);
            addCell(t,row,0,nameA,{},true);
            addCell(t,row,1,nameB,{},true);
            addCell(t,row,2,QString::number(kv.second),
                    kv.second>=5?QColor("#ee6644"):kv.second>=3?QColor("#eecc44"):QColor());
            addCell(t,row,3,QString::number(kv.second*1.5,'f',1),
                    kv.second*1.5>=7?QColor("#ee6644"):QColor());
            addCell(t,row,4,kv.second>=5?"Consider Blacklist or ↑coOccurrenceFactor":
                    kv.second>=3?"Monitor":"OK");
        }
        if (t->rowCount()==0) {
            t->insertRow(0);
            addCell(t,0,0,"No assignment history yet.", QColor("#5a7a9a"));
        }
        t->resizeRowsToContents();
        vl->addWidget(t);
    }

    // ────────────────────────────────────────────────────────────
    // TABLE 7: Boat utilisation
    // ────────────────────────────────────────────────────────────
    mkSec("Boat Utilisation across All Locked Assignments",
          "How often each boat has been used. Underused boats may signal they are "
          "too specialised (e.g. only Experienced rowers) or have restrictive whitelist settings.");
    {
        auto* t = mkTable({"Boat","Type","Cap","Steering","Propulsion","Times Used","Avg Crew Size"});
        QMap<int,int> usage, crewSize;
        for (const Assignment& a : m_assignments) {
            if (!a.isLocked()) continue;
            Assignment full = m_db->loadAssignment(a.id());
            for (auto it=full.boatRowerMap().constBegin();it!=full.boatRowerMap().constEnd();++it) {
                usage[it.key()]++;
                crewSize[it.key()] += it.value().size();
            }
        }
        for (const Boat& b : m_boats) {
            int row=t->rowCount(); t->insertRow(row);
            int u = usage.value(b.id(),0);
            addCell(t,row,0,b.name(),{},true);
            addCell(t,row,1,Boat::boatTypeToString(b.boatType()));
            addCell(t,row,2,QString::number(b.capacity()));
            addCell(t,row,3,Boat::steeringTypeToString(b.steeringType()));
            addCell(t,row,4,Boat::propulsionTypeToString(b.propulsionType()));
            addCell(t,row,5,QString::number(u),
                    u==0?QColor("#ee6644"):u<2?QColor("#eecc44"):QColor("#88ee88"));
            addCell(t,row,6,u>0?QString::number((double)crewSize.value(b.id(),0)/u,'f',1):"—");
        }
        t->resizeRowsToContents();
        vl->addWidget(t);
    }

    vl->addStretch();
}

// Called by buildAnalysisTab to build the graphical sub-tab content
void MainWindow::buildAnalysisGraphics(QVBoxLayout* vl) {
    auto mkSec = [&](const QString& t, const QString& desc="") {
        auto* l = new QLabel(t);
        l->setStyleSheet("color:#8fb4d8; font-weight:700; font-size:12px; "
                         "border-bottom:1px solid #2a3548; padding-bottom:3px;");
        vl->addWidget(l);
        if (!desc.isEmpty()) {
            auto* d = new QLabel(desc);
            d->setWordWrap(true);
            d->setStyleSheet("color:#5a7a9a; font-size:11px;");
            vl->addWidget(d);
        }
    };

    static const QColor kPal[] = {
        {100,200,140},{80,160,220},{220,160,60},{200,80,120},
        {160,100,220},{80,200,200},{220,140,80},{140,200,80}
    };

    // ── Chart 1: Rower skill distribution ────────────────────────
    mkSec("Rower Skill Distribution",
          "Count of rowers at each skill level. "
          "A heavily bottom-weighted distribution means most boats will be mixed, "
          "which is fine but reduces the impact of the Skill priority weight.");
    {
        QMap<QString,int> skillCount;
        for (auto& s : {"Student","Beginner","Experienced","Professional"}) skillCount[s]=0;
        for (const Rower& r:m_rowers) skillCount[Rower::skillToString(r.skill())]++;
        auto* chart = new BarChartWidget;
        chart->setTitle("Skill Distribution");
        chart->setBoatNames({"Student","Beginner","Experienced","Professional"});
        chart->setFixedHeight(200);
        chart->setSeries({{ "Count",
            {(double)skillCount["Student"],(double)skillCount["Beginner"],
             (double)skillCount["Experienced"],(double)skillCount["Professional"]},
            QColor(100,180,240) }});
        vl->addWidget(chart);
    }

    // ── Chart 2: Strength histogram ───────────────────────────────
    mkSec("Strength Distribution",
          "Distribution of rower strength values (1–10). "
          "Ideal: bell-curve centred around 5. A bimodal distribution "
          "(many 1s and many 10s) will produce high strengthVariance penalties.");
    {
        QList<double> counts(11, 0);
        QList<QString> labels;
        for (int i=0;i<=10;++i) labels << (i==0?"—":QString::number(i));
        for (const Rower& r:m_rowers) if(r.strength()>=0&&r.strength()<=10) counts[r.strength()]+=1;
        auto* chart = new BarChartWidget;
        chart->setTitle("Strength Histogram");
        chart->setBoatNames(labels);
        chart->setFixedHeight(200);
        chart->setSeries({{"Count", counts, QColor(100,200,140)}});
        vl->addWidget(chart);
    }

    // ── Chart 3: Rower attribute fill radar ───────────────────────
    mkSec("Attribute Fill Rate Radar",
          "How completely the attribute fields are filled in. "
          "Each axis goes from 0% (no rowers have this attribute set) "
          "to 100% (all rowers have it set). "
          "Attributes with low fill rates have minimal effect on generation.");
    {
        int n = m_rowers.size();
        if (n>0) {
            auto pct = [&](std::function<bool(const Rower&)> fn) {
                int c=0; for (const Rower& r:m_rowers) if(fn(r)) c++;
                return c/(double)n;
            };
            QList<double> vals = {
                pct([](const Rower& r){return r.strength()>0;}),
                pct([](const Rower& r){return r.strokeLength()>0;}),
                pct([](const Rower& r){return r.bodySize()>0;}),
                pct([](const Rower& r){return r.ageBand()>0;}),
                pct([](const Rower& r){return r.attrGrp1()>0;}),
                pct([](const Rower& r){return r.attrVal1()>0;}),
            };
            auto* chart = new RadarChartWidget;
            chart->setTitle("Attribute Fill Rate");
            chart->setAxes({"Strength","Stroke Len","Body Size","Age Band","GrpAttr1","ValAttr1"});
            chart->setFixedHeight(360);
            chart->setSeries({{"All Rowers", vals, QColor(100,200,140)}});
            vl->addWidget(chart);
        }
    }

    // ── Chart 4: Co-occurrence heatmap ────────────────────────────
    mkSec("Co-occurrence Heatmap — Top 12 Rowers by Assignment Count",
          "Darker cells = two rowers have shared a boat more often. "
          "The diagonal is always 0. High off-diagonal values mean those pairs "
          "are frequently grouped together and will accumulate co-occurrence penalties.");
    {
        // Take up to 12 most-active rowers by appearance count
        QMap<int,int> appear;
        auto co = m_db->loadCoOccurrence();
        for (auto it=co.constBegin();it!=co.constEnd();++it) {
            appear[it.key().first] += it.value();
            appear[it.key().second] += it.value();
        }
        QList<QPair<int,int>> sorted;
        for (auto it=appear.constBegin();it!=appear.constEnd();++it)
            sorted << qMakePair(it.value(), it.key());
        std::sort(sorted.rbegin(),sorted.rend());
        int take = qMin(12, sorted.size());
        QList<int> topIds;
        for (int i=0;i<take;++i) topIds << sorted[i].second;

        QList<QString> names;
        for (int id:topIds) {
            QString n=QString("R#%1").arg(id);
            for (const Rower& r:m_rowers) if(r.id()==id){n=r.name().left(10);break;}
            names<<n;
        }
        QList<QList<double>> mat;
        for (int i=0;i<take;++i) {
            QList<double> row;
            for (int j=0;j<take;++j) {
                if (i==j) { row<<0; continue; }
                int a=qMin(topIds[i],topIds[j]), b=qMax(topIds[i],topIds[j]);
                row << co.value({a,b},0);
            }
            mat << row;
        }
        if (!names.isEmpty()) {
            auto* hm = new HeatmapWidget;
            hm->setTitle("Co-occurrence Matrix");
            hm->setRowLabels(names);
            hm->setColLabels(names);
            hm->setValues(mat);
            hm->setLowColour(QColor(10,21,32));
            hm->setHighColour(QColor(220,80,60));
            hm->setFixedHeight(qMax(200, take*28+60));
            vl->addWidget(hm);
        } else {
            vl->addWidget(new QLabel("<i style='color:#5a7a9a;'>No assignment history yet.</i>"));
        }
    }

    // ── Chart 5: Score trend across locked assignments ─────────────
    mkSec("Average Score Trend across Locked Assignments",
          "Average team score per assignment over time (oldest → newest). "
          "An upward trend means your Expert Settings tuning is improving assignments. "
          "A downward trend may indicate rower mix changes or tighter constraints.");
    {
        QList<double> scores;
        QList<QString> labels;
        ScoringPriority defP;
        AssignmentGenerator gen;
        // Process oldest first (m_assignments is newest-first)
        QList<Assignment> sorted = m_assignments;
        std::reverse(sorted.begin(), sorted.end());
        for (const Assignment& a : sorted) {
            if (!a.isLocked()) continue;
            Assignment full = m_db->loadAssignment(a.id());
            auto details = gen.computeScoreDetails(full, m_boats, m_rowers, defP);
            if (details.isEmpty()) continue;
            double avg=0; for (auto& d:details) avg+=d.totalScore;
            scores << avg/details.size();
            labels << a.name().left(8)+"…";
        }
        if (!scores.isEmpty()) {
            auto* chart = new BarChartWidget;
            chart->setTitle("Avg Score Trend");
            chart->setBoatNames(labels);
            chart->setFixedHeight(220);
            chart->setSeries({{"Avg Score", scores, QColor(100,200,140)}});
            vl->addWidget(chart);
        } else {
            vl->addWidget(new QLabel("<i style='color:#5a7a9a;'>Lock assignments to see score trends.</i>"));
        }
    }
}

#include "mainwindow.h"
#include "assignmentdialog.h"
#include "assignmentgenerator.h"

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
    m_tabs->addTab(buildOptionsTab(),        "Options");
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
    //      8=Strength,9=StrokeLen,10=BodySize,11=Attr3,12=GrpAttr1,13=GrpAttr2,14=ValAttr1,15=ValAttr2
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
    m_rowerTable->setItemDelegateForColumn(11, new SpinBoxDelegate(0, 10, m_rowerTable)); // Attr3
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
    auto* wlBtn   = new QPushButton("Rower Whitelist...");
    auto* blBtn   = new QPushButton("Rower Blacklist...");
    auto* bwlBtn  = new QPushButton("Boat Whitelist...");
    auto* bblBtn  = new QPushButton("Boat Blacklist...");

    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(delBtn);
    btnLayout->addSpacing(20);
    btnLayout->addWidget(wlBtn);
    btnLayout->addWidget(blBtn);
    btnLayout->addWidget(bwlBtn);
    btnLayout->addWidget(bblBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(addBtn,  &QPushButton::clicked, this, &MainWindow::onAddRower);
    connect(delBtn,  &QPushButton::clicked, this, &MainWindow::onDeleteRower);
    connect(wlBtn,   &QPushButton::clicked, this, &MainWindow::onEditWhitelist);
    connect(blBtn,   &QPushButton::clicked, this, &MainWindow::onEditBlacklist);
    connect(bwlBtn,  &QPushButton::clicked, this, &MainWindow::onEditBoatWhitelist);
    connect(bblBtn,  &QPushButton::clicked, this, &MainWindow::onEditBoatBlacklist);
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

    // Two-tab view: text (default) and table
    auto* viewTabs = new QTabWidget;

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

        if (m_distAssignmentCombo)
            m_distAssignmentCombo->addItem(
                QString("%1  (%2)").arg(a.name())
                    .arg(a.createdAt().toString("dd.MM.yyyy")));
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

void MainWindow::onEditWhitelist() {
    auto sel = m_rowerTable->selectionModel()->selectedRows();
    if (sel.isEmpty()) { statusBar()->showMessage("Select a rower first.", 2000); return; }
    int row = sel.first().row();
    Rower rower = m_rowerModel->rowerAt(row);
    editList("Whitelist — " + rower.name(),
             "Check rowers who MUST row in the same boat as " + rower.name() + ":",
             rower, m_rowers, true, this);
    m_rowerModel->updateRower(row, rower);
    m_db->saveRower(rower);
    m_rowers = m_rowerModel->rowers();
    statusBar()->showMessage("Whitelist saved for " + rower.name(), 2000);
}

void MainWindow::onEditBlacklist() {
    auto sel = m_rowerTable->selectionModel()->selectedRows();
    if (sel.isEmpty()) { statusBar()->showMessage("Select a rower first.", 2000); return; }
    int row = sel.first().row();
    Rower rower = m_rowerModel->rowerAt(row);
    editList("Blacklist — " + rower.name(),
             "Check rowers who must NEVER row in the same boat as " + rower.name() + ":",
             rower, m_rowers, false, this);
    m_rowerModel->updateRower(row, rower);
    m_db->saveRower(rower);
    m_rowers = m_rowerModel->rowers();
    statusBar()->showMessage("Blacklist saved for " + rower.name(), 2000);
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
        statusBar()->showMessage("This assignment is locked — unlock it first to edit.", 3000);
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
        if (needsRoles && chosenObmann == -1 && !rowerIds.isEmpty())
            text += "  (no Obmann designated)\n";

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
    if (!m_distTable) return;
    m_distUpdating = true;
    m_distTable->setRowCount(0);
    if (index < 0 || index >= m_assignments.size()) { m_distUpdating = false; return; }

    const Assignment& a = m_assignments.at(index);
    Assignment full = m_db->loadAssignment(a.id());
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

    // Connect to printer if not already connected
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

    // Build the printable text from the assignment (same format as clipboard)
    QString printText;
    printText += m_currentAssignment.name() + "\n";
    printText += m_currentAssignment.createdAt().toString("dd.MM.yyyy hh:mm") + "\n";
    printText += QString("=").repeated(32) + "\n\n";

    const auto& map = m_currentAssignment.boatRowerMap();
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        printText += QString("== %1 ==\n").arg(boatDescription(it.key()));
        const QList<int>& rowerIds = it.value();

        Boat foundBoat;
        for (const Boat& b : m_boats) if (b.id() == it.key()) { foundBoat = b; break; }
        bool isSteered = foundBoat.id() != -1 && foundBoat.steeringType() == SteeringType::Steered;
        bool needsRoles = (foundBoat.id() == -1 || foundBoat.capacity() > 2);

        // Pick Obmann
        int chosenObmann = -1;
        if (needsRoles) {
            QList<int> ob;
            for (int rid : rowerIds)
                for (const Rower& r : m_rowers)
                    if (r.id() == rid && r.isObmann()) { ob << rid; break; }
            if (!ob.isEmpty()) {
                chosenObmann = ob[QRandomGenerator::global()->bounded(
                    static_cast<quint32>(ob.size()))];
            }
        }

        // Pick Steerer
        int chosenSteerer = -1;
        if (needsRoles && isSteered) {
            QList<int> st;
            for (int rid : rowerIds)
                for (const Rower& r : m_rowers)
                    if (r.id() == rid && r.canSteer() && rid != chosenObmann) { st << rid; break; }
            if (st.isEmpty())
                for (int rid : rowerIds)
                    for (const Rower& r : m_rowers)
                        if (r.id() == rid && r.canSteer()) { st << rid; break; }
            if (!st.isEmpty())
                chosenSteerer = st[QRandomGenerator::global()->bounded(
                    static_cast<quint32>(st.size()))];
        }

        if (needsRoles && chosenObmann == -1)
            printText += "  *** No Obmann! ***\n";

        if (chosenObmann != -1) {
            for (const Rower& r : m_rowers) {
                if (r.id() != chosenObmann) continue;
                QString tag = "[Obmann]";
                if (chosenObmann == chosenSteerer) tag += " [Steering]";
                printText += QString("  %1 %2\n").arg(r.name(), -18).arg(tag);
                break;
            }
        }
        for (int rid : rowerIds) {
            if (rid == chosenObmann) continue;
            for (const Rower& r : m_rowers) {
                if (r.id() != rid) continue;
                QString tag = (rid == chosenSteerer) ? " [Steering]" : "";
                printText += QString("  %1%2\n").arg(r.name(), -18).arg(tag);
                break;
            }
        }
        printText += "\n";
    }

    // Print the requested number of copies
    bool success = true;
    for (int copy = 0; copy < copies && success; ++copy) {
        statusBar()->showMessage(QString("Printing copy %1 of %2...").arg(copy+1).arg(copies), 0);
        success = m_printer.printText(printText);
    }

    if (!success) {
        QMessageBox::warning(this, "Print Error",
            QString("Printing failed on copy:\n\n%1").arg(m_printer.statusMessage()));
        statusBar()->showMessage("Print failed.", 3000);
    } else {
        statusBar()->showMessage(
            copies == 1 ? "Printed." : QString("Printed %1 copies.").arg(copies), 3000);
        m_printBtn->setText("Printed!");
        QTimer::singleShot(2000, this, [this]() { m_printBtn->setText("Print"); });
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

void MainWindow::onEditBoatWhitelist() {
    int row = m_rowerTable->currentIndex().row();
    if (row < 0) return;
    Rower r = m_rowerModel->rowerAt(row);
    QList<int> wl = r.boatWhitelist();
    editBoatList(this, "Boat Whitelist for " + r.name(), m_boats, wl, [&]() {
        r.setBoatWhitelist(wl);
        m_rowerModel->updateRower(row, r);
        m_db->saveRower(r);
        m_rowers[row] = r;
        statusBar()->showMessage("Boat whitelist saved for " + r.name(), 2000);
    });
}

void MainWindow::onEditBoatBlacklist() {
    int row = m_rowerTable->currentIndex().row();
    if (row < 0) return;
    Rower r = m_rowerModel->rowerAt(row);
    QList<int> bl = r.boatBlacklist();
    editBoatList(this, "Boat Blacklist for " + r.name(), m_boats, bl, [&]() {
        r.setBoatBlacklist(bl);
        m_rowerModel->updateRower(row, r);
        m_db->saveRower(r);
        m_rowers[row] = r;
        statusBar()->showMessage("Boat blacklist saved for " + r.name(), 2000);
    });
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
        a.setLocked(newLocked);
        m_db->setAssignmentLocked(id, newLocked);
        // Update list item display
        QString prefix = newLocked ? "🔒 " : "";
        item->setText(prefix + QString("%1\n%2")
            .arg(a.name())
            .arg(a.createdAt().toString("dd.MM.yyyy hh:mm")));
        if (m_currentAssignment.id() == id) m_currentAssignment.setLocked(newLocked);
        statusBar()->showMessage(
            newLocked ? "Assignment locked — double-click editing disabled."
                      : "Assignment unlocked.",
            3000);
        break;
    }
}

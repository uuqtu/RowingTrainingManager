#include "assignmentdialog.h"
#include "assignmentgenerator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QMessageBox>
#include <QDateTime>
#include <QTextEdit>
#include <QSplitter>
#include <QTabWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QFont>
#include <QInputDialog>
#include <QRandomGenerator>
#include <algorithm>

// Colour used to dim items that are claimed by a group
static const QString kDimColor  = "#44556688";   // rgba-ish (not used directly but noted)
static const QColor  kUsedBg    = QColor(0x1a, 0x40, 0x1a);   // dark green tint = claimed
static const QColor  kNormalBg  = QColor(0, 0, 0, 0);

// ---------------------------------------------------------------
AssignmentDialog::AssignmentDialog(const QList<Boat>& boats,
                                   const QList<Rower>& rowers,
                                   QWidget* parent)
    : QDialog(parent), m_boats(boats), m_rowers(rowers)
{
    setWindowTitle("Generate New Assignment");
    setMinimumSize(960, 720);
    resize(1100, 800);
    setupUi();
}

// ---------------------------------------------------------------
void AssignmentDialog::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setSpacing(10);
    root->setContentsMargins(14, 14, 14, 14);

    // Name row
    auto* nameRow = new QHBoxLayout;
    nameRow->addWidget(new QLabel("Assignment name:"));
    m_nameEdit = new QLineEdit;
    m_nameEdit->setText(QString("Assignment %1")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm")));
    nameRow->addWidget(m_nameEdit, 1);
    root->addLayout(nameRow);

    // Splitter: left = tabs, right = preview
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setChildrenCollapsible(false);

    m_tabs = new QTabWidget;
    m_tabs->addTab(buildGroupsTab(),    "1 — Groups");
    m_tabs->addTab(buildSelectionTab(), "2 — Boats & Rowers");
    m_tabs->addTab(buildPriorityTab(),  "3 — Priority");
    m_tabs->addTab(buildOptionsTab(),   "4 — Options");
    splitter->addWidget(m_tabs);

    // Right: preview with Text / Table tabs
    auto* previewW = new QWidget;
    auto* previewVL = new QVBoxLayout(previewW);
    previewVL->setContentsMargins(0, 0, 0, 0);
    previewVL->addWidget(new QLabel("Preview  (generated result):"));

    auto* previewTabs = new QTabWidget;

    m_previewEdit = new QTextEdit;
    m_previewEdit->setReadOnly(true);
    m_previewEdit->setPlaceholderText("Click 'Generate' to see the result here before saving…");
    QFont mono("Courier New", 11);
    m_previewEdit->setFont(mono);
    previewTabs->addTab(m_previewEdit, "Text");

    m_previewTable = new QTableWidget(0, 0);
    m_previewTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_previewTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_previewTable->horizontalHeader()->setDefaultSectionSize(120);
    m_previewTable->verticalHeader()->setVisible(false);
    m_previewTable->setStyleSheet(
        "QTableWidget { gridline-color: #2a3548; }"
        "QHeaderView::section { background:#1a2535; color:#8fb4d8; font-weight:600; "
        "padding:4px; border:1px solid #2a3548; }");
    previewTabs->addTab(m_previewTable, "Table");

    previewVL->addWidget(previewTabs);
    splitter->addWidget(previewW);
    splitter->setSizes({520, 480});
    root->addWidget(splitter, 1);

    // Status
    m_statusLabel = new QLabel;
    m_statusLabel->setWordWrap(true);
    root->addWidget(m_statusLabel);

    // Buttons
    auto* btnRow = new QHBoxLayout;
    m_generateBtn = new QPushButton("Generate");
    m_generateBtn->setObjectName("primaryBtn");
    m_checkBtn = new QPushButton("Check");
    m_acceptBtn = new QPushButton("Accept & Save");
    m_acceptBtn->setEnabled(false);
    auto* cancelBtn = new QPushButton("Cancel");
    btnRow->addWidget(m_generateBtn);
    btnRow->addWidget(m_checkBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_acceptBtn);
    btnRow->addWidget(cancelBtn);
    root->addLayout(btnRow);

    connect(m_generateBtn, &QPushButton::clicked, this, &AssignmentDialog::onGenerate);
    connect(m_checkBtn,    &QPushButton::clicked, this, &AssignmentDialog::onCheck);
    connect(m_acceptBtn,   &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn,     &QPushButton::clicked, this, &QDialog::reject);
}

// ================================================================
// TAB 1 — GROUPS
// ================================================================
QWidget* AssignmentDialog::buildGroupsTab()
{
    auto* w  = new QWidget;
    auto* vl = new QVBoxLayout(w);
    vl->setSpacing(8);

    auto* info = new QLabel(
        "Define groups of rowers who <b>must</b> share the same boat. "
        "Optionally pin each group to a specific boat.<br>"
        "Groups are session-only and are not saved to the database.");
    info->setWordWrap(true);
    info->setStyleSheet("color:#8fb4d8; font-style:italic;");
    vl->addWidget(info);

    auto* mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->setChildrenCollapsible(false);

    // ---- Left: group list ----
    auto* leftW  = new QWidget;
    auto* leftVL = new QVBoxLayout(leftW);
    leftVL->setContentsMargins(0, 0, 4, 0);
    leftVL->addWidget(new QLabel("Groups:"));
    m_groupList = new QListWidget;
    leftVL->addWidget(m_groupList, 1);

    auto* groupBtns = new QHBoxLayout;
    auto* addGroupBtn = new QPushButton("＋ New");
    addGroupBtn->setObjectName("primaryBtn");
    m_removeGroupBtn = new QPushButton("✕ Remove");
    m_removeGroupBtn->setObjectName("dangerBtn");
    m_removeGroupBtn->setEnabled(false);
    groupBtns->addWidget(addGroupBtn);
    groupBtns->addWidget(m_removeGroupBtn);
    leftVL->addLayout(groupBtns);
    mainSplitter->addWidget(leftW);

    // ---- Right: group editor ----
    auto* rightW  = new QWidget;
    auto* rightVL = new QVBoxLayout(rightW);
    rightVL->setContentsMargins(4, 0, 0, 0);

    // Boat assignment for group
    auto* boatRow = new QHBoxLayout;
    m_groupBoatLabel = new QLabel("Pinned boat:");
    m_groupBoatLabel->setEnabled(false);
    m_groupBoatCombo = new QComboBox;
    m_groupBoatCombo->setEnabled(false);
    boatRow->addWidget(m_groupBoatLabel);
    boatRow->addWidget(m_groupBoatCombo, 1);
    rightVL->addLayout(boatRow);

    // Members
    rightVL->addWidget(new QLabel("Members of this group:"));
    m_groupMemberList = new QListWidget;
    m_groupMemberList->setSelectionMode(QAbstractItemView::SingleSelection);
    rightVL->addWidget(m_groupMemberList, 1);

    m_removeFromGroupBtn = new QPushButton("⬆  Remove selected from group");
    m_removeFromGroupBtn->setEnabled(false);
    rightVL->addWidget(m_removeFromGroupBtn);

    // Available rowers
    rightVL->addWidget(new QLabel("Add rower to group  (already-grouped rowers are hidden):"));
    m_availRowerList = new QListWidget;
    m_availRowerList->setSelectionMode(QAbstractItemView::SingleSelection);
    rightVL->addWidget(m_availRowerList, 1);

    m_addToGroupBtn = new QPushButton("⬇  Add selected rower to group");
    m_addToGroupBtn->setEnabled(false);
    rightVL->addWidget(m_addToGroupBtn);

    mainSplitter->addWidget(rightW);
    mainSplitter->setSizes({180, 340});
    vl->addWidget(mainSplitter, 1);

    // Initial populate of avail rowers (all visible, no groups yet)
    refreshAvailRowerList();

    connect(addGroupBtn,          &QPushButton::clicked,              this, &AssignmentDialog::onAddGroup);
    connect(m_removeGroupBtn,     &QPushButton::clicked,              this, &AssignmentDialog::onRemoveGroup);
    connect(m_groupList,          &QListWidget::itemSelectionChanged, this, &AssignmentDialog::onGroupSelectionChanged);
    connect(m_addToGroupBtn,      &QPushButton::clicked,              this, &AssignmentDialog::onAddRowerToGroup);
    connect(m_removeFromGroupBtn, &QPushButton::clicked,              this, &AssignmentDialog::onRemoveRowerFromGroup);
    connect(m_groupBoatCombo,     QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AssignmentDialog::onGroupBoatChanged);

    return w;
}

// ================================================================
// TAB 2 — BOATS & ROWERS
// ================================================================
QWidget* AssignmentDialog::buildSelectionTab()
{
    auto* w  = new QWidget;
    auto* vl = new QVBoxLayout(w);
    vl->setSpacing(10);

    auto* info = new QLabel(
        "Check boats and rowers to include in the random assignment.<br>"
        "<span style='color:#66cc66;'>■</span> Items already assigned via a group are shown but cannot be re-selected.");
    info->setWordWrap(true);
    info->setStyleSheet("color:#8fb4d8; font-style:italic;");
    vl->addWidget(info);

    // Auto-select boats
    m_autoBoatsCheck = new QCheckBox("Select boats automatically to fit the selected rowers");
    m_autoBoatsCheck->setStyleSheet("font-weight:600;");
    vl->addWidget(m_autoBoatsCheck);

    // Boats
    auto* boatGroup = new QGroupBox("Boats to fill  (check to include)");
    auto* boatVL    = new QVBoxLayout(boatGroup);
    m_boatList = new QListWidget;
    m_boatList->setSelectionMode(QAbstractItemView::NoSelection);
    for (const Boat& b : m_boats) {
        auto* item = new QListWidgetItem(
            QString("%1  [%2 | Cap:%3 | %4 | %5]")
                .arg(b.name())
                .arg(Boat::boatTypeToString(b.boatType()))
                .arg(b.capacity())
                .arg(Boat::steeringTypeToString(b.steeringType()))
                .arg(Boat::propulsionTypeToString(b.propulsionType()))
        );
        item->setData(Qt::UserRole, b.id());
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
        m_boatList->addItem(item);
    }
    boatVL->addWidget(m_boatList);
    auto* boatBtns = new QHBoxLayout;
    auto* selAllBoats  = new QPushButton("Select all");
    auto* selNoneBoats = new QPushButton("Select none");
    boatBtns->addWidget(selAllBoats);
    boatBtns->addWidget(selNoneBoats);
    boatBtns->addStretch();
    boatVL->addLayout(boatBtns);
    vl->addWidget(boatGroup);

    // Rowers
    auto* rowerGroup = new QGroupBox("Available rowers  (check to include)");
    auto* rowerVL    = new QVBoxLayout(rowerGroup);
    m_rowerList = new QListWidget;
    m_rowerList->setSelectionMode(QAbstractItemView::NoSelection);
    for (const Rower& r : m_rowers) {
        QStringList tags;
        if (r.canSteer())  tags << "Steer";
        if (r.isObmann())  tags << "Obmann";
        tags << Boat::propulsionTypeToString(r.propulsionAbility());
        auto* item = new QListWidgetItem(
            QString("%1  (Sk:%2 Co:%3 | %4)")
                .arg(r.name()).arg(Rower::skillToString(r.skill())).arg(Rower::compatToString(r.compatibility()))
                .arg(tags.join(", "))
        );
        item->setData(Qt::UserRole, r.id());
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
        m_rowerList->addItem(item);
    }
    rowerVL->addWidget(m_rowerList, 1);
    auto* rowerBtns = new QHBoxLayout;
    auto* selAllRowers  = new QPushButton("Select all");
    auto* selNoneRowers = new QPushButton("Select none");
    rowerBtns->addWidget(selAllRowers);
    rowerBtns->addWidget(selNoneRowers);
    rowerBtns->addStretch();
    rowerVL->addLayout(rowerBtns);
    vl->addWidget(rowerGroup, 1);

    connect(selAllBoats,   &QPushButton::clicked, this, &AssignmentDialog::onSelectAllBoats);
    connect(selNoneBoats,  &QPushButton::clicked, this, &AssignmentDialog::onSelectNoneBoats);
    connect(selAllRowers,  &QPushButton::clicked, this, &AssignmentDialog::onSelectAllRowers);
    connect(selNoneRowers, &QPushButton::clicked, this, &AssignmentDialog::onSelectNoneRowers);

    connect(m_boatList,  &QListWidget::itemChanged, this, &AssignmentDialog::onSelectionChanged);
    connect(m_rowerList, &QListWidget::itemChanged, this, &AssignmentDialog::onSelectionChanged);
    connect(m_autoBoatsCheck, &QCheckBox::toggled, this, &AssignmentDialog::onAutoSelectBoatsToggled);

    return w;
}

// ================================================================
// TAB 3 — PRIORITY
// ================================================================
QWidget* AssignmentDialog::buildPriorityTab()
{
    auto* w  = new QWidget;
    auto* vl = new QVBoxLayout(w);
    auto* info = new QLabel(
        "Drag rows or use arrows to set scoring priority.<br>"
        "<b>First = highest weight</b> (4×). Second = 2×. Third = 1×.");
    info->setWordWrap(true);
    info->setStyleSheet("color:#8fb4d8; margin-bottom:6px;");
    vl->addWidget(info);

    m_priorityList = new QListWidget;
    m_priorityList->setDragDropMode(QAbstractItemView::InternalMove);
    m_priorityList->setDefaultDropAction(Qt::MoveAction);
    for (const QString& label : {"Skill", "Compatibility", "Propulsion match",
                                  "Stroke Length", "Body Size"}) {
        auto* item = new QListWidgetItem(label);
        item->setFlags(item->flags() | Qt::ItemIsDragEnabled);
        m_priorityList->addItem(item);
    }
    vl->addWidget(m_priorityList);

    auto* arrows = new QHBoxLayout;
    auto* upBtn   = new QPushButton("▲  Up");
    auto* downBtn = new QPushButton("▼  Down");
    arrows->addWidget(upBtn);
    arrows->addWidget(downBtn);
    arrows->addStretch();
    vl->addLayout(arrows);

    vl->addSpacing(16);

    // Training mode
    m_trainingCheck = new QCheckBox("Training mode");
    m_trainingCheck->setStyleSheet("font-weight: 600; color: #8fb4d8;");
    vl->addWidget(m_trainingCheck);
    auto* trainingInfo = new QLabel(
        "In training mode, skill level and compatibility are completely ignored.\n"
        "Only propulsion ability, whitelist/blacklist, and attribute proximity are used.");
    trainingInfo->setWordWrap(true);
    trainingInfo->setStyleSheet("color: #556677; font-style: italic;");
    vl->addWidget(trainingInfo);

    vl->addSpacing(12);

    // Crazy mode
    m_crazyCheck = new QCheckBox("Crazy mode");
    m_crazyCheck->setStyleSheet("font-weight: 600; color: #cc6644;");
    vl->addWidget(m_crazyCheck);
    auto* crazyInfo = new QLabel(
        "Crazy mode ignores EVERYTHING — blacklists, whitelists, propulsion, skill, "
        "compatibility, groups. Rowers are distributed to boats completely at random. "
        "Fun at parties.");
    crazyInfo->setWordWrap(true);
    crazyInfo->setStyleSheet("color: #774433; font-style: italic;");
    vl->addWidget(crazyInfo);

    vl->addStretch();

    connect(upBtn,   &QPushButton::clicked, this, &AssignmentDialog::onMovePriorityUp);
    connect(downBtn, &QPushButton::clicked, this, &AssignmentDialog::onMovePriorityDown);
    return w;
}

// ================================================================
// GROUPS LOGIC
// ================================================================

QList<int> AssignmentDialog::claimedRowerIds() const
{
    QList<int> claimed;
    for (const RowingGroup& g : m_groups)
        for (int id : g.rowerIds)
            if (!claimed.contains(id)) claimed << id;
    return claimed;
}

QList<int> AssignmentDialog::claimedBoatIds() const
{
    QList<int> claimed;
    for (const RowingGroup& g : m_groups)
        if (g.boatId != -1 && !claimed.contains(g.boatId))
            claimed << g.boatId;
    return claimed;
}

void AssignmentDialog::refreshAvailRowerList()
{
    if (!m_availRowerList) return;
    QList<int> claimed = claimedRowerIds();
    m_availRowerList->clear();
    for (const Rower& r : m_rowers) {
        if (claimed.contains(r.id())) continue;   // hide already-grouped rowers
        auto* item = new QListWidgetItem(r.name());
        item->setData(Qt::UserRole, r.id());
        m_availRowerList->addItem(item);
    }
}

void AssignmentDialog::refreshGroupBoatCombo()
{
    if (!m_groupBoatCombo) return;
    // Block signals while rebuilding
    m_groupBoatCombo->blockSignals(true);
    m_groupBoatCombo->clear();
    m_groupBoatCombo->addItem("— none (generator decides) —", -1);

    int groupRow = m_groupList->currentRow();
    int currentPinnedBoatId = (groupRow >= 0 && groupRow < m_groups.size())
        ? m_groups.at(groupRow).boatId : -1;

    QList<int> claimed = claimedBoatIds();

    for (const Boat& b : m_boats) {
        // Offer the boat if it's not claimed by ANOTHER group
        bool claimedByOther = claimed.contains(b.id()) && b.id() != currentPinnedBoatId;
        if (claimedByOther) continue;
        m_groupBoatCombo->addItem(
            QString("%1  [Cap:%2 | %3 | %4]")
                .arg(b.name()).arg(b.capacity())
                .arg(Boat::steeringTypeToString(b.steeringType()))
                .arg(Boat::propulsionTypeToString(b.propulsionType())),
            b.id()
        );
    }

    // Restore selection
    int selectIndex = 0;
    for (int i = 0; i < m_groupBoatCombo->count(); ++i) {
        if (m_groupBoatCombo->itemData(i).toInt() == currentPinnedBoatId) {
            selectIndex = i;
            break;
        }
    }
    m_groupBoatCombo->setCurrentIndex(selectIndex);
    m_groupBoatCombo->blockSignals(false);
}

void AssignmentDialog::refreshSelectionTabStates()
{
    if (!m_boatList || !m_rowerList) return;

    QList<int> claimedRowers = claimedRowerIds();
    QList<int> claimedBoats  = claimedBoatIds();

    // Update rower list in Tab 2
    for (int i = 0; i < m_rowerList->count(); ++i) {
        auto* item = m_rowerList->item(i);
        int rid = item->data(Qt::UserRole).toInt();
        bool claimed = claimedRowers.contains(rid);

        // Find which group claimed this rower for the tooltip
        QString groupName;
        for (const RowingGroup& g : m_groups)
            if (g.rowerIds.contains(rid)) { groupName = g.name; break; }

        if (claimed) {
            // Grey out, uncheck, disable
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled & ~Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
            item->setForeground(QColor("#557755"));
            // Update label to show which group
            for (const Rower& r : m_rowers) {
                if (r.id() == rid) {
                    QStringList tags;
                    if (r.canSteer())  tags << "Steer";
                    if (r.isObmann())  tags << "Obmann";
                    tags << Boat::propulsionTypeToString(r.propulsionAbility());
                    item->setText(QString("✔ %1  (Sk:%2 Co:%3 | %4)  [Group: %5]")
                        .arg(r.name()).arg(Rower::skillToString(r.skill())).arg(Rower::compatToString(r.compatibility()))
                        .arg(tags.join(", ")).arg(groupName));
                    break;
                }
            }
        } else {
            // Restore
            item->setFlags(item->flags() | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
            item->setForeground(QColor("#e8edf5"));
            if (item->checkState() == Qt::Unchecked) item->setCheckState(Qt::Checked);
            // Restore original label
            for (const Rower& r : m_rowers) {
                if (r.id() == rid) {
                    QStringList tags;
                    if (r.canSteer())  tags << "Steer";
                    if (r.isObmann())  tags << "Obmann";
                    tags << Boat::propulsionTypeToString(r.propulsionAbility());
                    item->setText(QString("%1  (Sk:%2 Co:%3 | %4)")
                        .arg(r.name()).arg(Rower::skillToString(r.skill())).arg(Rower::compatToString(r.compatibility()))
                        .arg(tags.join(", ")));
                    break;
                }
            }
        }
    }

    // Update boat list in Tab 2
    for (int i = 0; i < m_boatList->count(); ++i) {
        auto* item = m_boatList->item(i);
        int bid = item->data(Qt::UserRole).toInt();
        bool claimed = claimedBoats.contains(bid);

        QString groupName;
        for (const RowingGroup& g : m_groups)
            if (g.boatId == bid) { groupName = g.name; break; }

        if (claimed) {
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled & ~Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
            item->setForeground(QColor("#557755"));
            for (const Boat& b : m_boats) {
                if (b.id() == bid) {
                    item->setText(QString("✔ %1  [%2 | Cap:%3 | %4 | %5]  [Group: %6]")
                        .arg(b.name())
                        .arg(Boat::boatTypeToString(b.boatType()))
                        .arg(b.capacity())
                        .arg(Boat::steeringTypeToString(b.steeringType()))
                        .arg(Boat::propulsionTypeToString(b.propulsionType()))
                        .arg(groupName));
                    break;
                }
            }
        } else {
            item->setFlags(item->flags() | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
            item->setForeground(QColor("#e8edf5"));
            if (item->checkState() == Qt::Unchecked) item->setCheckState(Qt::Checked);
            for (const Boat& b : m_boats) {
                if (b.id() == bid) {
                    item->setText(QString("%1  [%2 | Cap:%3 | %4 | %5]")
                        .arg(b.name())
                        .arg(Boat::boatTypeToString(b.boatType()))
                        .arg(b.capacity())
                        .arg(Boat::steeringTypeToString(b.steeringType()))
                        .arg(Boat::propulsionTypeToString(b.propulsionType())));
                    break;
                }
            }
        }
    }
}

void AssignmentDialog::onAddGroup()
{
    bool ok;
    QString name = QInputDialog::getText(this, "New Group", "Group name:",
        QLineEdit::Normal, QString("Group %1").arg(m_groups.size() + 1), &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    RowingGroup g;
    g.name = name.trimmed();
    m_groups.append(g);

    auto* item = new QListWidgetItem(g.name);
    item->setData(Qt::UserRole, m_groups.size() - 1);
    m_groupList->addItem(item);
    m_groupList->setCurrentItem(item);
}

void AssignmentDialog::onRemoveGroup()
{
    int row = m_groupList->currentRow();
    if (row < 0 || row >= m_groups.size()) return;
    m_groups.removeAt(row);
    delete m_groupList->takeItem(row);
    for (int i = 0; i < m_groupList->count(); ++i)
        m_groupList->item(i)->setData(Qt::UserRole, i);
    onGroupSelectionChanged();
    refreshAvailRowerList();
    refreshSelectionTabStates();
}

void AssignmentDialog::onGroupSelectionChanged()
{
    int row = m_groupList->currentRow();
    bool valid = (row >= 0 && row < m_groups.size());
    m_removeGroupBtn->setEnabled(valid);
    m_addToGroupBtn->setEnabled(valid);
    m_removeFromGroupBtn->setEnabled(false);
    m_groupBoatLabel->setEnabled(valid);
    m_groupBoatCombo->setEnabled(valid);
    m_groupMemberList->clear();

    if (!valid) return;

    // Populate member list
    const RowingGroup& g = m_groups.at(row);
    for (int id : g.rowerIds) {
        for (const Rower& r : m_rowers) {
            if (r.id() == id) {
                auto* item = new QListWidgetItem(r.name());
                item->setData(Qt::UserRole, id);
                m_groupMemberList->addItem(item);
                break;
            }
        }
    }
    connect(m_groupMemberList, &QListWidget::itemSelectionChanged, this, [this]() {
        m_removeFromGroupBtn->setEnabled(m_groupMemberList->currentItem() != nullptr);
    });

    refreshGroupBoatCombo();
}

void AssignmentDialog::onAddRowerToGroup()
{
    int groupRow = m_groupList->currentRow();
    if (groupRow < 0 || groupRow >= m_groups.size()) return;
    auto* item = m_availRowerList->currentItem();
    if (!item) return;
    int rowerId = item->data(Qt::UserRole).toInt();

    RowingGroup& g = m_groups[groupRow];
    if (g.rowerIds.contains(rowerId)) return;

    // If group is pinned to a boat, check capacity before adding
    if (g.boatId != -1) {
        int boatCap = 0;
        for (const Boat& b : m_boats)
            if (b.id() == g.boatId) { boatCap = b.capacity(); break; }
        if (boatCap > 0 && g.rowerIds.size() >= boatCap) {
            m_statusLabel->setText(
                QString("<font color='#ff6666'>Cannot add: group \"%1\" is pinned to a "
                        "boat with capacity %2 and already has %3 member(s).</font>")
                    .arg(g.name).arg(boatCap).arg(g.rowerIds.size()));
            return;
        }
    }

    g.rowerIds.append(rowerId);

    // Update the group label — show fill status if pinned
    QString label = g.name;
    if (g.boatId != -1) {
        int boatCap = 0;
        for (const Boat& b : m_boats) if (b.id() == g.boatId) { boatCap = b.capacity(); break; }
        label = QString("%1  (%2/%3)").arg(g.name).arg(g.rowerIds.size()).arg(boatCap);
    } else {
        label = QString("%1  (%2 rowers)").arg(g.name).arg(g.rowerIds.size());
    }
    m_groupList->item(groupRow)->setText(label);

    // Clear any previous capacity warning
    m_statusLabel->setText({});

    refreshAvailRowerList();
    onGroupSelectionChanged();
    refreshSelectionTabStates();
}

void AssignmentDialog::onRemoveRowerFromGroup()
{
    int groupRow = m_groupList->currentRow();
    if (groupRow < 0 || groupRow >= m_groups.size()) return;
    auto* item = m_groupMemberList->currentItem();
    if (!item) return;
    int rowerId = item->data(Qt::UserRole).toInt();
    m_groups[groupRow].rowerIds.removeAll(rowerId);

    RowingGroup& g = m_groups[groupRow];
    QString label;
    if (g.boatId != -1) {
        int boatCap = 0;
        for (const Boat& b : m_boats) if (b.id() == g.boatId) { boatCap = b.capacity(); break; }
        label = g.rowerIds.isEmpty()
            ? g.name
            : QString("%1  (%2/%3)").arg(g.name).arg(g.rowerIds.size()).arg(boatCap);
    } else {
        label = g.rowerIds.isEmpty()
            ? g.name
            : QString("%1  (%2 rowers)").arg(g.name).arg(g.rowerIds.size());
    }
    m_groupList->item(groupRow)->setText(label);
    m_statusLabel->setText({});

    refreshAvailRowerList();
    onGroupSelectionChanged();
    refreshSelectionTabStates();
}

void AssignmentDialog::onGroupBoatChanged(int comboIndex)
{
    int groupRow = m_groupList->currentRow();
    if (groupRow < 0 || groupRow >= m_groups.size()) return;
    int boatId = m_groupBoatCombo->itemData(comboIndex).toInt();
    RowingGroup& g = m_groups[groupRow];
    g.boatId = boatId;

    // Validate fit and give immediate feedback
    if (boatId == -1) {
        // Unpinned — revert label to plain rower count
        m_groupList->item(groupRow)->setText(
            g.rowerIds.isEmpty()
                ? g.name
                : QString("%1  (%2 rowers)").arg(g.name).arg(g.rowerIds.size()));
        m_statusLabel->setText({});
    } else {
        int boatCap = 0;
        QString boatName;
        for (const Boat& b : m_boats)
            if (b.id() == boatId) { boatCap = b.capacity(); boatName = b.name(); break; }

        // Update label to show fill ratio
        m_groupList->item(groupRow)->setText(
            QString("%1  (%2/%3)").arg(g.name).arg(g.rowerIds.size()).arg(boatCap));

        // Capacity feedback
        if (g.rowerIds.size() > boatCap) {
            m_statusLabel->setText(
                QString("<font color='#ff6666'>Group \"%1\" has %2 members but "
                        "\"%3\" only holds %4. Please remove %5 member(s).</font>")
                    .arg(g.name).arg(g.rowerIds.size())
                    .arg(boatName).arg(boatCap)
                    .arg(g.rowerIds.size() - boatCap));
        } else if (g.rowerIds.size() == boatCap) {
            m_statusLabel->setText(
                QString("<font color='#66ff99'>Group \"%1\" exactly fills \"%2\" "
                        "(%3/%3).</font>")
                    .arg(g.name).arg(boatName).arg(boatCap));
        } else {
            int missing = boatCap - g.rowerIds.size();
            m_statusLabel->setText(
                QString("<font color='#ffaa44'>Group \"%1\" has %2/%3 members for "
                        "\"%4\" — add %5 more, or the generator will fill the rest.</font>")
                    .arg(g.name).arg(g.rowerIds.size()).arg(boatCap)
                    .arg(boatName).arg(missing));
        }
    }

    refreshSelectionTabStates();
}

// ================================================================
// RESTORE FROM SAVED ASSIGNMENT
// ================================================================
void AssignmentDialog::loadFromAssignment(const Assignment& a)
{
    m_nameEdit->setText(a.name());

    // ---- Restore groups (Tab 1) ----
    m_groups.clear();
    m_groupList->clear();
    m_groupMemberList->clear();

    for (const SavedGroup& sg : a.groups()) {
        RowingGroup g;
        g.name     = sg.name;
        g.rowerIds = sg.rowerIds;
        g.boatId   = sg.boatId;
        m_groups.append(g);

        QString label = g.rowerIds.isEmpty()
            ? g.name
            : QString("%1  (%2 rowers)").arg(g.name).arg(g.rowerIds.size());
        auto* item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, m_groups.size() - 1);
        m_groupList->addItem(item);
    }

    // Rebuild avail rower list now that groups are set
    refreshAvailRowerList();

    // ---- Restore Tab 2: checked boats and rowers ----
    // First apply group-claimed state so we don't re-check claimed items
    refreshSelectionTabStates();

    // Boats: uncheck all free ones, then check those in checkedBoatIds
    QList<int> savedBoats  = a.checkedBoatIds();
    QList<int> claimedB    = claimedBoatIds();
    for (int i = 0; i < m_boatList->count(); ++i) {
        auto* item = m_boatList->item(i);
        int bid = item->data(Qt::UserRole).toInt();
        if (claimedB.contains(bid)) continue;   // managed by group state
        item->setCheckState(savedBoats.contains(bid) ? Qt::Checked : Qt::Unchecked);
    }

    // Rowers: uncheck all free ones, then check those in checkedRowerIds
    QList<int> savedRowers = a.checkedRowerIds();
    QList<int> claimedR    = claimedRowerIds();
    for (int i = 0; i < m_rowerList->count(); ++i) {
        auto* item = m_rowerList->item(i);
        int rid = item->data(Qt::UserRole).toInt();
        if (claimedR.contains(rid)) continue;
        item->setCheckState(savedRowers.contains(rid) ? Qt::Checked : Qt::Unchecked);
    }

    // ---- Restore priority order (Tab 3) ----
    QStringList prio = a.priorityOrder();
    // Strip the training flag token if present
    bool wasTraining = prio.removeAll("__training__") > 0;
    if (m_trainingCheck) m_trainingCheck->setChecked(wasTraining);

    if (!prio.isEmpty() && prio.size() == m_priorityList->count()) {
        // Re-order items to match saved order
        QList<QListWidgetItem*> items;
        for (int i = 0; i < m_priorityList->count(); ++i)
            items << m_priorityList->takeItem(0);
        for (const QString& label : prio) {
            for (QListWidgetItem* item : items) {
                if (item->text() == label) {
                    m_priorityList->addItem(item);
                    items.removeOne(item);
                    break;
                }
            }
        }
        // Any leftovers (shouldn't happen)
        for (QListWidgetItem* item : items)
            m_priorityList->addItem(item);
    }

    // Switch to groups tab so user sees the full restored state
    m_tabs->setCurrentIndex(0);
    updateGenerateEnabled();
}

// ================================================================
// SELECTION HELPERS & VALIDATION
// ================================================================

QList<Boat> AssignmentDialog::collectSelectedBoats() const
{
    QList<Boat> result;
    for (const RowingGroup& g : m_groups) {
        if (g.boatId == -1) continue;
        for (const Boat& b : m_boats)
            if (b.id() == g.boatId) { result.append(b); break; }
    }
    QList<int> already;
    for (const Boat& b : result) already << b.id();
    for (int i = 0; i < m_boatList->count(); ++i) {
        auto* item = m_boatList->item(i);
        if (item->checkState() != Qt::Checked) continue;
        int id = item->data(Qt::UserRole).toInt();
        if (!already.contains(id))
            for (const Boat& b : m_boats)
                if (b.id() == id) { result.append(b); break; }
    }
    return result;
}

QList<Rower> AssignmentDialog::collectSelectedRowers() const
{
    QList<Rower> result;
    QList<int> claimed = claimedRowerIds();
    for (const RowingGroup& g : m_groups)
        for (int rid : g.rowerIds)
            for (const Rower& r : m_rowers)
                if (r.id() == rid) { result.append(r); break; }
    for (int i = 0; i < m_rowerList->count(); ++i) {
        auto* item = m_rowerList->item(i);
        if (item->checkState() != Qt::Checked) continue;
        int id = item->data(Qt::UserRole).toInt();
        if (!claimed.contains(id))
            for (const Rower& r : m_rowers)
                if (r.id() == id) { result.append(r); break; }
    }
    return result;
}

QStringList AssignmentDialog::runChecks(const QList<Boat>& boats,
                                         const QList<Rower>& rowers) const
{
    QStringList issues;
    if (boats.isEmpty()) { issues << "No boats selected."; return issues; }
    if (rowers.isEmpty()) { issues << "No rowers selected."; return issues; }

    // ── Group / boat capacity consistency ────────────────────────
    for (const RowingGroup& g : m_groups) {
        if (g.boatId == -1) continue;   // unpinned — generator handles size
        int boatCap = 0;
        QString boatName;
        for (const Boat& b : m_boats)
            if (b.id() == g.boatId) { boatCap = b.capacity(); boatName = b.name(); break; }
        if (boatCap == 0) continue;
        if (g.rowerIds.size() > boatCap)
            issues << QString("Group \"%1\" has %2 member(s) but boat \"%3\" only holds %4. "
                              "Remove %5 member(s) from the group.")
                          .arg(g.name).arg(g.rowerIds.size())
                          .arg(boatName).arg(boatCap)
                          .arg(g.rowerIds.size() - boatCap);
        // Under-filled groups are fine — the generator fills remaining seats
    }

    // Equipment limits — counted per rower (one oar per rower for skulls, one oar per rower for riemen)
    int maxSkulls  = m_globalScullOars;
    int maxRiemen  = m_globalSweepOars;
    if (maxSkulls > 0 || maxRiemen > 0) {
        int neededSkulls = 0, neededRiemen = 0;
        for (const Boat& b : boats) {
            if (b.propulsionType() == PropulsionType::Skull)  neededSkulls  += b.capacity();
            if (b.propulsionType() == PropulsionType::Riemen) neededRiemen += b.capacity();
        }
        if (maxSkulls  > 0 && neededSkulls  > maxSkulls)
            issues << QString("Equipment limit exceeded: selected scull boats need %1 scull oar(s) "
                              "but only %2 available.")
                          .arg(neededSkulls).arg(maxSkulls);
        if (maxRiemen > 0 && neededRiemen > maxRiemen)
            issues << QString("Equipment limit exceeded: selected sweep boats need %1 sweep oar(s) "
                              "but only %2 available.")
                          .arg(neededRiemen).arg(maxRiemen);
    }

    int totalCapacity = 0;
    for (const Boat& b : boats) totalCapacity += b.capacity();

    if (rowers.size() < totalCapacity)
        issues << QString("Not enough rowers: %1 selected, %2 seats to fill.")
                      .arg(rowers.size()).arg(totalCapacity);
    else if (rowers.size() > totalCapacity)
        issues << QString("%1 rower(s) will not be assigned (%2 selected, %3 seats).")
                      .arg(rowers.size() - totalCapacity).arg(rowers.size()).arg(totalCapacity);

    // Obmann note (capacity > 2 only): not a hard block — generator assigns someone regardless
    int boatsNeedingObmann  = 0;
    int boatsNeedingSteerer = 0;
    for (const Boat& b : boats) {
        if (b.capacity() > 2) {
            boatsNeedingObmann++;
            // Foot-Steered (SteeringType::Steered) = needs a dedicated foot-steerer in the boat
            // Hand-Steered (SteeringType::NonSteered) = no dedicated steerer needed
            if (b.steeringType() == SteeringType::Steered) boatsNeedingSteerer++;
        }
    }

    if (boatsNeedingObmann > 0) {
        int obmannCount = 0;
        for (const Rower& r : rowers) if (r.isObmann()) obmannCount++;
        if (obmannCount == 0)
            issues << QString("Note: no rower with Obmann ability selected for %1 boat(s) "
                              "with capacity > 2 — the generator will assign someone anyway.")
                          .arg(boatsNeedingObmann);
    }

    if (boatsNeedingSteerer > 0) {
        int steererCount = 0;
        for (const Rower& r : rowers) if (r.canSteer()) steererCount++;
        if (steererCount < boatsNeedingSteerer)
            issues << QString("Need at least %1 person(s) with [Foot Steerer] ability for "
                              "Foot-Steered boat(s) with capacity > 2, only %2 available.")
                          .arg(boatsNeedingSteerer).arg(steererCount);
    }

    // Propulsion compatibility
    for (const Boat& b : boats) {
        if (b.propulsionType() == PropulsionType::Both) continue;
        int capable = 0;
        for (const Rower& r : rowers)
            if (r.canRowPropulsion(b.propulsionType())) capable++;
        if (capable < b.capacity())
            issues << QString("Boat \"%1\" needs %2 rower(s) capable of %3, only %4 qualified.")
                          .arg(b.name()).arg(b.capacity())
                          .arg(Boat::propulsionTypeToString(b.propulsionType())).arg(capable);
    }

    // Skill / boat-type advisory: warn if Racing boats may need to take beginners
    for (const Boat& b : boats) {
        if (b.boatType() != BoatType::Racing) continue;
        int beginnerCount = 0;
        for (const Rower& r : rowers)
            if (r.skill() == SkillLevel::Student || r.skill() == SkillLevel::Beginner)
                beginnerCount++;
        int racingCapacity = 0;
        for (const Boat& rb : boats)
            if (rb.boatType() == BoatType::Racing) racingCapacity += rb.capacity();
        int nonBeginnerCount = rowers.size() - beginnerCount;
        if (nonBeginnerCount < racingCapacity)
            issues << QString("Warning: %1 racing seat(s) but only %2 Experienced/Professional "
                              "rower(s) — %3 beginner(s) may end up in a racing boat.")
                          .arg(racingCapacity).arg(nonBeginnerCount)
                          .arg(racingCapacity - nonBeginnerCount);
    }

    return issues;
}

void AssignmentDialog::updateGenerateEnabled()
{
    // Crazy mode always enables Generate if there's at least one boat and one rower
    bool isCrazy = m_crazyCheck && m_crazyCheck->isChecked();

    QList<Boat>  boats  = collectSelectedBoats();
    QList<Rower> rowers = collectSelectedRowers();

    int totalCapacity = 0;
    for (const Boat& b : boats) totalCapacity += b.capacity();

    bool capacityOk = !boats.isEmpty() && !rowers.isEmpty()
                      && (isCrazy || rowers.size() == totalCapacity);

    // Also block if equipment limits are violated (hard stop even for crazy mode)
    bool equipOk = true;
    if (!isCrazy) {
        int maxSkulls  = m_globalScullOars;
        int maxRiemen  = m_globalSweepOars;
        if (maxSkulls > 0) {
            int needSkulls = 0;
            for (const Boat& b : boats) if (b.propulsionType() == PropulsionType::Skull) needSkulls += b.capacity();
            if (needSkulls > maxSkulls) equipOk = false;
        }
        if (maxRiemen > 0) {
            int needRiemen = 0;
            for (const Boat& b : boats) if (b.propulsionType() == PropulsionType::Riemen) needRiemen += b.capacity();
            if (needRiemen > maxRiemen) equipOk = false;
        }
    }
    m_generateBtn->setEnabled(capacityOk && equipOk);

    if (isCrazy) {
        m_statusLabel->setText(
            QString("<font color='#cc6644'>CRAZY MODE — %1 rower(s) into %2 seats, purely random.</font>")
                .arg(rowers.size()).arg(totalCapacity));
        return;
    }

    if (boats.isEmpty() || rowers.isEmpty())
        m_statusLabel->setText("<font color='#aaaaaa'>Select boats and rowers to begin.</font>");
    else if (rowers.size() < totalCapacity)
        m_statusLabel->setText(
            QString("<font color='#ffaa44'>%1 rower(s) / %2 seats — need %3 more.</font>")
                .arg(rowers.size()).arg(totalCapacity).arg(totalCapacity - rowers.size()));
    else if (rowers.size() > totalCapacity)
        m_statusLabel->setText(
            QString("<font color='#ffaa44'>%1 rower(s) / %2 seats — %3 will not be assigned.</font>")
                .arg(rowers.size()).arg(totalCapacity).arg(rowers.size() - totalCapacity));
    else
        m_statusLabel->setText(
            QString("<font color='#66ff99'>%1 rower(s) / %2 seats — OK. Press Check or Generate.</font>")
                .arg(rowers.size()).arg(totalCapacity));
}

void AssignmentDialog::onSelectionChanged()
{
    if (m_autoBoatsCheck && m_autoBoatsCheck->isChecked())
        autoSelectBoats();
    updateGenerateEnabled();
}

void AssignmentDialog::onCheck()
{
    QList<Boat>  boats  = collectSelectedBoats();
    QList<Rower> rowers = collectSelectedRowers();
    QStringList  issues = runChecks(boats, rowers);

    int totalCapacity = 0;
    for (const Boat& b : boats) totalCapacity += b.capacity();

    QString report;
    if (issues.isEmpty()) {
        report = "All preconditions met.\n";
    } else {
        report = QString("%1 issue(s) found:\n\n").arg(issues.size());
        for (const QString& issue : issues)
            report += "  • " + issue + "\n";
    }
    report += "\n";

    // Unassigned rowers when there are more rowers than seats
    if (!rowers.isEmpty() && rowers.size() > totalCapacity && totalCapacity > 0) {
        report += QString("Rowers without a seat (%1 over capacity — listed alphabetically):\n")
                      .arg(rowers.size() - totalCapacity);
        QList<Rower> sorted = rowers;
        std::sort(sorted.begin(), sorted.end(),
                  [](const Rower& a, const Rower& b){ return a.name() < b.name(); });
        for (int i = totalCapacity; i < sorted.size(); ++i)
            report += QString("  • %1\n").arg(sorted.at(i).name());
    } else if (!rowers.isEmpty() && rowers.size() < totalCapacity) {
        report += QString("Missing %1 rower(s) to fill all seats.\n")
                      .arg(totalCapacity - rowers.size());
    }

    m_previewEdit->setPlainText(report);
    if (!issues.isEmpty())
        m_statusLabel->setText(
            QString("<font color='#ff6666'>%1 issue(s) found — see preview panel.</font>")
                .arg(issues.size()));
    else
        m_statusLabel->setText("<font color='#66ff99'>All checks passed.</font>");
}

// ================================================================
// GENERATOR GLUE
// ================================================================
QList<Rower> AssignmentDialog::rowersWithGroupsApplied(const QList<Rower>& base) const
{
    QList<Rower> result = base;
    for (const RowingGroup& g : m_groups) {
        if (g.rowerIds.size() < 2) continue;
        for (Rower& r : result) {
            if (!g.rowerIds.contains(r.id())) continue;
            QList<int> wl = r.whitelist();
            for (int otherId : g.rowerIds)
                if (otherId != r.id() && !wl.contains(otherId))
                    wl.append(otherId);
            r.setWhitelist(wl);
        }
    }
    return result;
}

ScoringPriority AssignmentDialog::buildPriority() const
{
    ScoringPriority p;
    p.order.clear();
    for (int i = 0; i < m_priorityList->count(); ++i) {
        QString label = m_priorityList->item(i)->text();
        if (label == "Skill")              p.order << ScoringPriority::Skill;
        else if (label == "Compatibility") p.order << ScoringPriority::Compatibility;
        else if (label == "Stroke Length") p.order << ScoringPriority::StrokeLength;
        else if (label == "Body Size")     p.order << ScoringPriority::BodySize;
        else                               p.order << ScoringPriority::Propulsion;
    }
    p.trainingMode = m_trainingCheck && m_trainingCheck->isChecked();
    p.crazyMode    = m_crazyCheck    && m_crazyCheck->isChecked();
    p.coOccurrence = m_coOccurrence;
    return p;
}

// Shared role-annotation logic used by both formatPreview (dialog) and
// formatAssignmentText (main window). Returns {obmannId, steeringId}.
static QPair<int,int> pickRoles(const QList<int>& rowerIds, const QList<Rower>& allRowers,
                                 bool boatIsSteered)
{
    // Obmann: random pick from candidates
    QList<int> obmannCandidates;
    for (int rid : rowerIds)
        for (const Rower& r : allRowers)
            if (r.id() == rid && r.isObmann()) { obmannCandidates << rid; break; }
    int chosenObmann = -1;
    if (!obmannCandidates.isEmpty())
        chosenObmann = obmannCandidates[QRandomGenerator::global()->bounded(
            static_cast<quint32>(obmannCandidates.size()))];

    // Steering: only for steered boats — random pick from steerers (not the Obmann if possible)
    int chosenSteerer = -1;
    if (boatIsSteered) {
        QList<int> steererCandidates;
        for (int rid : rowerIds)
            for (const Rower& r : allRowers)
                if (r.id() == rid && r.canSteer() && rid != chosenObmann)
                    { steererCandidates << rid; break; }
        // Fall back to any steerer if needed
        if (steererCandidates.isEmpty())
            for (int rid : rowerIds)
                for (const Rower& r : allRowers)
                    if (r.id() == rid && r.canSteer())
                        { steererCandidates << rid; break; }
        if (!steererCandidates.isEmpty())
            chosenSteerer = steererCandidates[QRandomGenerator::global()->bounded(
                static_cast<quint32>(steererCandidates.size()))];
    }

    return {chosenObmann, chosenSteerer};
}

QString AssignmentDialog::formatPreview(const Assignment& a) const
{
    QString text;
    text += a.name() + "\n";
    text += QString("Generated: %1\n").arg(a.createdAt().toString("dd.MM.yyyy hh:mm:ss"));
    text += QString("═").repeated(54) + "\n\n";

    for (const RowingGroup& g : m_groups) {
        if (g.rowerIds.size() < 2 && g.boatId == -1) continue;
        QStringList names;
        for (int id : g.rowerIds)
            for (const Rower& r : m_rowers)
                if (r.id() == id) { names << r.name(); break; }
        QString boatName;
        if (g.boatId != -1)
            for (const Boat& b : m_boats)
                if (b.id() == g.boatId) { boatName = "  -> " + b.name(); break; }
        text += QString("  Group \"%1\": %2%3\n").arg(g.name).arg(names.join(", ")).arg(boatName);
    }
    if (!m_groups.isEmpty()) text += "\n";

    const auto& map = a.boatRowerMap();
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        int boatId = it.key();
        Boat foundBoat;
        for (const Boat& b : m_boats) if (b.id() == boatId) { foundBoat = b; break; }

        text += QString("====== %1 ======\n")
                    .arg(foundBoat.id() != -1 ? foundBoat.name() : QString("Boat#%1").arg(boatId));
        if (foundBoat.id() != -1)
            text += QString("  %1 | Cap:%2 | %3 | %4\n")
                        .arg(Boat::boatTypeToString(foundBoat.boatType()))
                        .arg(foundBoat.capacity())
                        .arg(Boat::steeringTypeToString(foundBoat.steeringType()))
                        .arg(Boat::propulsionTypeToString(foundBoat.propulsionType()));

        bool isSteered = foundBoat.id() != -1
                         && foundBoat.steeringType() == SteeringType::Steered;
        bool needsRoles = (foundBoat.id() == -1 || foundBoat.capacity() > 2);

        int chosenObmann  = -1;
        int chosenSteerer = -1;
        if (needsRoles) {
            auto [ob, st] = pickRoles(it.value(), m_rowers, isSteered);
            chosenObmann  = ob;
            chosenSteerer = st;
        }

        if (needsRoles && chosenObmann == -1)
            text += "  *** No Obmann available for this boat! ***\n";

        // Helper: collect group tags for a rower id
        auto groupTags = [&](int rid) -> QStringList {
            QStringList gt;
            for (const RowingGroup& g : m_groups)
                if (g.rowerIds.contains(rid))
                    gt << QString("[%1]").arg(g.name);
            return gt;
        };

        // Print Obmann first — include group tags too
        if (chosenObmann != -1) {
            // Find name — may not be in m_rowers if sick-filtered; fall back to ID
            QString obName;
            for (const Rower& r : m_rowers) if (r.id() == chosenObmann) { obName = r.name(); break; }
            if (obName.isEmpty()) obName = QString("Rower#%1").arg(chosenObmann);
            QStringList tags;
            tags << "[Obmann]";
            if (chosenObmann == chosenSteerer) tags << "[Steering]";
            tags << groupTags(chosenObmann);
            text += QString("  %1  %2\n").arg(obName, -22).arg(tags.join(" "));
        }

        for (int rid : it.value()) {
            if (rid == chosenObmann) continue;
            QString name;
            for (const Rower& r : m_rowers) if (r.id() == rid) { name = r.name(); break; }
            if (name.isEmpty()) name = QString("Rower#%1").arg(rid);
            QStringList tags;
            if (rid == chosenSteerer) tags << "[Steering]";
            tags << groupTags(rid);
            text += QString("  %1%2\n")
                        .arg(name, -22)
                        .arg(tags.isEmpty() ? QString() : "  " + tags.join(" "));
        }
        text += "\n";
    }
    return text;
}

void AssignmentDialog::onGenerate()
{
    QList<Boat>  selectedBoats  = collectSelectedBoats();
    QList<Rower> selectedRowers = collectSelectedRowers();

    ScoringPriority priority = buildPriority();

    if (selectedBoats.isEmpty()) {
        m_statusLabel->setText("<font color='#ff6666'>Please select at least one boat.</font>");
        return;
    }
    if (selectedRowers.isEmpty()) {
        m_statusLabel->setText("<font color='#ff6666'>Please select at least one rower.</font>");
        return;
    }

    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty())
        name = QString("Assignment %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm"));

    // ── Crazy mode: skip all normal checks ─────────────────────────
    if (priority.crazyMode) {
        AssignmentGenerator gen;
        GeneratorResult result = gen.generate(selectedBoats, selectedRowers, name, priority);
        m_assignment = result.assignment;
        m_assignment.setGroups({});
        m_assignment.setCheckedBoatIds({});
        m_assignment.setCheckedRowerIds({});
        m_assignment.setPriorityOrder({"__crazy__"});
        m_previewEdit->setPlainText(formatPreview(m_assignment));
        populatePreviewTable(m_assignment);
        m_statusLabel->setText("<font color='#cc6644'>Crazy mode — random distribution applied!</font>");
        m_acceptBtn->setEnabled(true);
        return;
    }

    // ── Normal mode: capacity check ─────────────────────────────────
    // Hard-block if any pinned group exceeds its boat's capacity
    for (const RowingGroup& g : m_groups) {
        if (g.boatId == -1) continue;
        int boatCap = 0;
        for (const Boat& b : m_boats) if (b.id() == g.boatId) { boatCap = b.capacity(); break; }
        if (boatCap > 0 && g.rowerIds.size() > boatCap) {
            m_statusLabel->setText(
                QString("<font color='#ff6666'>Group \"%1\" has %2 members but its pinned boat "
                        "only holds %3. Fix this in the Groups tab before generating.</font>")
                    .arg(g.name).arg(g.rowerIds.size()).arg(boatCap));
            m_previewEdit->clear();
            if (m_previewTable) { m_previewTable->clear(); m_previewTable->setRowCount(0); m_previewTable->setColumnCount(0); }
            m_acceptBtn->setEnabled(false);
            return;
        }
    }

    QStringList issues = runChecks(selectedBoats, selectedRowers);
    int totalCapacity = 0;
    for (const Boat& b : selectedBoats) totalCapacity += b.capacity();
    if (selectedRowers.size() != totalCapacity) {
        onCheck();
        return;
    }

    QList<Rower> effectiveRowers = rowersWithGroupsApplied(selectedRowers);

    // Pre-seed pinned groups
    // A pinned boat is FULLY pinned only when its group fills all seats.
    // If the group is smaller than the boat's capacity, the boat stays in
    // freeBoats so the generator can fill the remaining seats.
    QList<Boat> pinnedBoats, freeBoats;
    QList<int>  pinnedRowerIds;

    for (const RowingGroup& g : m_groups) {
        if (g.boatId == -1) continue;
        // Find the boat capacity
        int boatCap = 0;
        for (const Boat& b : selectedBoats)
            if (b.id() == g.boatId) { boatCap = b.capacity(); break; }
        // Only fully-pinned if group fills every seat
        bool fullyPinned = (boatCap > 0 && g.rowerIds.size() >= boatCap);
        if (fullyPinned) {
            for (const Boat& b : selectedBoats)
                if (b.id() == g.boatId) { pinnedBoats << b; break; }
        }
        for (int rid : g.rowerIds) pinnedRowerIds << rid;
    }

    for (const Boat& b : selectedBoats) {
        bool pinned = false;
        for (const Boat& pb : pinnedBoats) if (pb.id() == b.id()) { pinned = true; break; }
        if (!pinned) freeBoats << b;
    }

    // freeRowers = everyone not consumed by a fully-pinned group
    QList<Rower> freeRowers;
    for (const Rower& r : effectiveRowers)
        if (!pinnedRowerIds.contains(r.id())) freeRowers << r;

    // Pre-seed the assignment with pinned group members for ALL pinned groups
    // (both fully-pinned and partially-pinned)
    Assignment preSeeded;
    preSeeded.setName(name);
    preSeeded.setCreatedAt(QDateTime::currentDateTime());
    for (const RowingGroup& g : m_groups) {
        if (g.boatId == -1) continue;
        for (int rid : g.rowerIds)
            preSeeded.assignRowerToBoat(g.boatId, rid);
    }

    // For partially-pinned boats that remain in freeBoats, tell the generator
    // how many seats are already occupied so it fills only the remainder.
    // We do this by reducing the capacity of those boats for the generator call.
    QList<Boat> generatorBoats;
    for (const Boat& b : freeBoats) {
        // Count how many seats are already pre-seeded in this boat
        int preSeatedCount = 0;
        const auto& psMap = preSeeded.boatRowerMap();
        if (psMap.contains(b.id())) preSeatedCount = psMap[b.id()].size();

        if (preSeatedCount > 0 && preSeatedCount < b.capacity()) {
            // Partially filled — give generator a reduced-capacity copy
            Boat partial = b;
            partial.setCapacity(b.capacity() - preSeatedCount);
            generatorBoats << partial;
        } else if (preSeatedCount == 0) {
            generatorBoats << b;
        }
        // If preSeatedCount >= capacity: fully covered, skip (shouldn't happen here)
    }

    if (!generatorBoats.isEmpty()) {
        AssignmentGenerator gen;
        GeneratorResult result = gen.generate(generatorBoats, freeRowers, name, priority);

        // Build context for diagnostic (full picture, not just free portion)
        AssignmentGenerator::DiagContext ctx;
        ctx.allSelectedRowers = selectedRowers;
        ctx.allSelectedBoats  = selectedBoats;        ctx.maxSkullPairs     = m_globalScullOars;
        ctx.maxRiemenPairs    = m_globalSweepOars;
        for (const RowingGroup& g : m_groups) {
            if (g.rowerIds.isEmpty()) continue;
            QStringList names;
            for (int rid : g.rowerIds)
                for (const Rower& r : m_rowers)
                    if (r.id() == rid) { names << r.name(); break; }
            QString boatPart;
            if (g.boatId != -1)
                for (const Boat& b : m_boats)
                    if (b.id() == g.boatId) { boatPart = " -> " + b.name(); break; }
            ctx.groupSummaries << QString("\"%1\": %2%3").arg(g.name).arg(names.join(", ")).arg(boatPart);
        }
        for (const SteeringOnlyEntry& e : m_steeringOnly) {
            QString rName;
            for (const Rower& r : m_rowers) if (r.id() == e.rowerId) { rName = r.name(); break; }
            QString bName = "any";
            if (e.boatId != -1)
                for (const Boat& b : m_boats) if (b.id() == e.boatId) { bName = b.name(); break; }
            ctx.steeringOnlySummaries << QString("%1 -> %2").arg(rName).arg(bName);
        }

        if (!result.success) {
            QString diag = gen.diagnose(generatorBoats, freeRowers, priority, ctx);
            QString report = "GENERATION FAILED\n";
            report += QString("Error: %1\n\n").arg(result.errorMessage);
            report += diag;
            m_previewEdit->setPlainText(report);
            m_statusLabel->setText(
                "<font color='#ff6666'>Generation failed — see preview panel for full diagnostic.</font>");
            m_acceptBtn->setEnabled(false);
            return;
        }
        for (auto it = result.assignment.boatRowerMap().constBegin();
             it != result.assignment.boatRowerMap().constEnd(); ++it)
            for (int rid : it.value())
                preSeeded.assignRowerToBoat(it.key(), rid);

        // If constraints were relaxed, append the diagnostic note
        if (!result.errorMessage.isEmpty()) {
            m_assignment = preSeeded;
            QString previewText = formatPreview(m_assignment);
            previewText += "\n--- NOTE: Constraints were relaxed ---\n";
            previewText += result.errorMessage + "\n\n";
            previewText += gen.diagnose(generatorBoats, freeRowers, priority, ctx);
            m_previewEdit->setPlainText(previewText);
            // Save state and show success
            goto stateCapture;
        }
    } else if (preSeeded.boatRowerMap().isEmpty()) {
        m_statusLabel->setText("<font color='#ff6666'>No boats or rowers to assign.</font>");
        return;
    }

    m_assignment = preSeeded;
    m_previewEdit->setPlainText(formatPreview(m_assignment));
    populatePreviewTable(m_assignment);

stateCapture:

    // ---- Persist dialog state so double-click restore works exactly ----
    QList<SavedGroup> savedGroups;
    for (const RowingGroup& g : m_groups) {
        SavedGroup sg;
        sg.name     = g.name;
        sg.rowerIds = g.rowerIds;
        sg.boatId   = g.boatId;
        savedGroups << sg;
    }
    m_assignment.setGroups(savedGroups);

    QList<int> checkedBoats;
    QList<int> claimedB = claimedBoatIds();
    for (int i = 0; i < m_boatList->count(); ++i) {
        auto* item = m_boatList->item(i);
        if (item->checkState() == Qt::Checked) {
            int bid = item->data(Qt::UserRole).toInt();
            if (!claimedB.contains(bid)) checkedBoats << bid;
        }
    }
    m_assignment.setCheckedBoatIds(checkedBoats);

    QList<int> checkedRowers;
    QList<int> claimedR = claimedRowerIds();
    for (int i = 0; i < m_rowerList->count(); ++i) {
        auto* item = m_rowerList->item(i);
        if (item->checkState() == Qt::Checked) {
            int rid = item->data(Qt::UserRole).toInt();
            if (!claimedR.contains(rid)) checkedRowers << rid;
        }
    }
    m_assignment.setCheckedRowerIds(checkedRowers);

    QStringList prio;
    for (int i = 0; i < m_priorityList->count(); ++i)
        prio << m_priorityList->item(i)->text();
    if (m_trainingCheck && m_trainingCheck->isChecked())
        prio << "__training__";
    m_assignment.setPriorityOrder(prio);

    QString statusMsg = "<font color='#66ff99'>Generated — review preview, then Accept & Save.</font>";
    if (!issues.isEmpty())
        statusMsg = QString("<font color='#ffaa44'>Generated with %1 warning(s) — see Check for details. Accept to save.</font>")
                        .arg(issues.size());
    m_statusLabel->setText(statusMsg);
    m_acceptBtn->setEnabled(true);
}

// ================================================================
// TAB 4 — OPTIONS
// ================================================================
QWidget* AssignmentDialog::buildOptionsTab()
{
    auto* w  = new QWidget;
    auto* vl = new QVBoxLayout(w);
    vl->setSpacing(12);

    // Equipment limits info (now configured in main application Options tab)
    auto* equipInfo = new QLabel(
        "Equipment limits (scull oars / sweep oars) are configured in the main "
        "application Options tab and apply automatically here.");
    equipInfo->setWordWrap(true);
    equipInfo->setStyleSheet("color:#5a7a9a; font-style:italic; padding:4px;");
    vl->addWidget(equipInfo);

    // ---- Steering-only people ----
    auto* soGroup = new QGroupBox("Steering-only people");
    auto* soVL    = new QVBoxLayout(soGroup);
    auto* soInfo  = new QLabel(
        "Steering-only people ride along but do not occupy a rower seat — "
        "they are excluded from the capacity count.\n"
        "You can optionally pin them to a specific boat.");
    soInfo->setWordWrap(true);
    soInfo->setStyleSheet("color:#5a7a9a; font-style:italic;");
    soVL->addWidget(soInfo);

    m_steeringOnlyTable = new QTableWidget(0, 2);
    m_steeringOnlyTable->setHorizontalHeaderLabels({"Rower", "Pinned Boat"});
    m_steeringOnlyTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_steeringOnlyTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_steeringOnlyTable->verticalHeader()->setVisible(false);
    m_steeringOnlyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_steeringOnlyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_steeringOnlyTable->setMaximumHeight(150);
    soVL->addWidget(m_steeringOnlyTable);

    auto* addRow = new QHBoxLayout;
    addRow->addWidget(new QLabel("Rower:"));
    m_soRowerCombo = new QComboBox;
    for (const Rower& r : m_rowers)
        m_soRowerCombo->addItem(r.name(), r.id());
    addRow->addWidget(m_soRowerCombo, 1);
    addRow->addWidget(new QLabel("Boat (optional):"));
    m_soBoatCombo = new QComboBox;
    m_soBoatCombo->addItem("— any —", -1);
    for (const Boat& b : m_boats)
        m_soBoatCombo->addItem(b.name(), b.id());
    addRow->addWidget(m_soBoatCombo, 1);
    auto* addSoBtn = new QPushButton("Add");
    addSoBtn->setObjectName("primaryBtn");
    addRow->addWidget(addSoBtn);
    auto* delSoBtn = new QPushButton("Remove");
    delSoBtn->setObjectName("dangerBtn");
    addRow->addWidget(delSoBtn);
    soVL->addLayout(addRow);
    vl->addWidget(soGroup);

    vl->addStretch();

    connect(addSoBtn, &QPushButton::clicked, this, &AssignmentDialog::onAddSteeringOnly);
    connect(delSoBtn, &QPushButton::clicked, this, &AssignmentDialog::onRemoveSteeringOnly);

    return w;
}

void AssignmentDialog::onAddSteeringOnly()
{
    if (!m_soRowerCombo || !m_soBoatCombo || !m_steeringOnlyTable) return;
    int rowerId = m_soRowerCombo->currentData().toInt();
    int boatId  = m_soBoatCombo->currentData().toInt();

    // Avoid duplicates
    for (const SteeringOnlyEntry& e : m_steeringOnly)
        if (e.rowerId == rowerId) return;

    SteeringOnlyEntry e;
    e.rowerId = rowerId;
    e.boatId  = boatId;
    m_steeringOnly.append(e);

    int row = m_steeringOnlyTable->rowCount();
    m_steeringOnlyTable->insertRow(row);
    m_steeringOnlyTable->setItem(row, 0, new QTableWidgetItem(m_soRowerCombo->currentText()));
    m_steeringOnlyTable->setItem(row, 1, new QTableWidgetItem(
        boatId == -1 ? "any" : m_soBoatCombo->currentText()));
    m_steeringOnlyTable->item(row, 0)->setData(Qt::UserRole, rowerId);

    // Remove from selection lists — steering-only people are not rowers
    for (int i = 0; i < m_rowerList->count(); ++i) {
        auto* item = m_rowerList->item(i);
        if (item->data(Qt::UserRole).toInt() == rowerId) {
            item->setCheckState(Qt::Unchecked);
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled & ~Qt::ItemIsUserCheckable);
            item->setForeground(QColor("#886644"));
            item->setText(item->text() + "  [Steering only]");
        }
    }
    updateGenerateEnabled();
}

void AssignmentDialog::onRemoveSteeringOnly()
{
    if (!m_steeringOnlyTable) return;
    int row = m_steeringOnlyTable->currentRow();
    if (row < 0 || row >= m_steeringOnly.size()) return;
    int rowerId = m_steeringOnly.at(row).rowerId;
    m_steeringOnly.removeAt(row);
    m_steeringOnlyTable->removeRow(row);

    // Re-enable in rower list
    for (int i = 0; i < m_rowerList->count(); ++i) {
        auto* item = m_rowerList->item(i);
        if (item->data(Qt::UserRole).toInt() == rowerId) {
            item->setFlags(item->flags() | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
            item->setForeground(QColor("#e8edf5"));
            // Strip the "[Steering only]" suffix
            QString t = item->text();
            t.remove("  [Steering only]");
            item->setText(t);
            item->setCheckState(Qt::Checked);
        }
    }
    updateGenerateEnabled();
}

// ================================================================
// AUTO-SELECT BOATS
// ================================================================
void AssignmentDialog::autoSelectBoats()
{
    if (!m_boatList || !m_autoBoatsCheck || !m_autoBoatsCheck->isChecked()) return;

    // Determine available rower count (checked + group members, minus steering-only)
    QList<Rower> rowers = collectSelectedRowers();
    int rowerCount = rowers.size();
    if (rowerCount == 0) return;

    // Equipment limits
    int maxSkulls  = m_globalScullOars;
    int maxRiemen  = m_globalSweepOars;
    int usedSkulls = 0, usedRiemen = 0;

    // Already pinned boats from groups
    QList<int> pinnedIds = claimedBoatIds();
    int pinnedCapacity = 0;
    for (int bid : pinnedIds)
        for (const Boat& b : m_boats)
            if (b.id() == bid) { pinnedCapacity += b.capacity(); break; }

    int needed = rowerCount - pinnedCapacity;
    if (needed <= 0) {
        // Uncheck all free boats
        for (int i = 0; i < m_boatList->count(); ++i) {
            auto* item = m_boatList->item(i);
            if (!pinnedIds.contains(item->data(Qt::UserRole).toInt()))
                item->setCheckState(Qt::Unchecked);
        }
        return;
    }

    // Count equipment already used by pinned boats
    for (int bid : pinnedIds)
        for (const Boat& b : m_boats)
            if (b.id() == bid) {
                if (b.propulsionType() == PropulsionType::Skull)  usedSkulls += b.capacity() / 2;
                if (b.propulsionType() == PropulsionType::Riemen) usedRiemen += b.capacity() / 2;
                break;
            }

    // Greedily select free boats until capacity is met
    // Prefer boats whose propulsion fits within equipment limits
    // Uncheck all free boats first
    m_boatList->blockSignals(true);
    for (int i = 0; i < m_boatList->count(); ++i) {
        auto* item = m_boatList->item(i);
        if (!pinnedIds.contains(item->data(Qt::UserRole).toInt()))
            item->setCheckState(Qt::Unchecked);
    }

    int filled = 0;
    // Two passes: first prefer exact capacity match, then fill greedily
    for (int pass = 0; pass < 2 && filled < needed; ++pass) {
        for (int i = 0; i < m_boatList->count() && filled < needed; ++i) {
            auto* item = m_boatList->item(i);
            int bid = item->data(Qt::UserRole).toInt();
            if (pinnedIds.contains(bid) || item->checkState() == Qt::Checked) continue;

            for (const Boat& b : m_boats) {
                if (b.id() != bid) continue;
                // Equipment check
                int needSkulls  = (b.propulsionType() == PropulsionType::Skull)  ? b.capacity() / 2 : 0;
                int needRiemen  = (b.propulsionType() == PropulsionType::Riemen) ? b.capacity() / 2 : 0;
                bool skullOk  = (maxSkulls  == 0) || (usedSkulls  + needSkulls  <= maxSkulls);
                bool riemenOk = (maxRiemen  == 0) || (usedRiemen  + needRiemen  <= maxRiemen);
                if (!skullOk || !riemenOk) break;

                // Pass 0: only select if it doesn't overshoot
                if (pass == 0 && filled + b.capacity() > needed) break;

                item->setCheckState(Qt::Checked);
                filled    += b.capacity();
                usedSkulls  += needSkulls;
                usedRiemen  += needRiemen;
                break;
            }
        }
    }
    m_boatList->blockSignals(false);
    updateGenerateEnabled();
}

void AssignmentDialog::onAutoSelectBoatsToggled(bool checked)
{
    if (!m_boatList) return;
    // Disable manual editing of the boat list when auto mode is on
    for (int i = 0; i < m_boatList->count(); ++i) {
        auto* item = m_boatList->item(i);
        if (checked)
            item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
        else
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    }
    if (checked) autoSelectBoats();
    updateGenerateEnabled();
}

// ================================================================
// SELECTION TAB BUTTON HANDLERS
// ================================================================
void AssignmentDialog::onSelectAllBoats() {
    QList<int> claimed = claimedBoatIds();
    for (int i = 0; i < m_boatList->count(); ++i) {
        auto* item = m_boatList->item(i);
        if (!claimed.contains(item->data(Qt::UserRole).toInt()))
            item->setCheckState(Qt::Checked);
    }
}
void AssignmentDialog::onSelectNoneBoats() {
    QList<int> claimed = claimedBoatIds();
    for (int i = 0; i < m_boatList->count(); ++i) {
        auto* item = m_boatList->item(i);
        if (!claimed.contains(item->data(Qt::UserRole).toInt()))
            item->setCheckState(Qt::Unchecked);
    }
}
void AssignmentDialog::onSelectAllRowers() {
    QList<int> claimed = claimedRowerIds();
    for (int i = 0; i < m_rowerList->count(); ++i) {
        auto* item = m_rowerList->item(i);
        if (!claimed.contains(item->data(Qt::UserRole).toInt()))
            item->setCheckState(Qt::Checked);
    }
}
void AssignmentDialog::onSelectNoneRowers() {
    QList<int> claimed = claimedRowerIds();
    for (int i = 0; i < m_rowerList->count(); ++i) {
        auto* item = m_rowerList->item(i);
        if (!claimed.contains(item->data(Qt::UserRole).toInt()))
            item->setCheckState(Qt::Unchecked);
    }
}
void AssignmentDialog::onMovePriorityUp() {
    int row = m_priorityList->currentRow();
    if (row <= 0) return;
    auto* item = m_priorityList->takeItem(row);
    m_priorityList->insertItem(row - 1, item);
    m_priorityList->setCurrentRow(row - 1);
}
void AssignmentDialog::onMovePriorityDown() {
    int row = m_priorityList->currentRow();
    if (row < 0 || row >= m_priorityList->count() - 1) return;
    auto* item = m_priorityList->takeItem(row);
    m_priorityList->insertItem(row + 1, item);
    m_priorityList->setCurrentRow(row + 1);
}

// ---------------------------------------------------------------
// Table view of preview (dialog)
// ---------------------------------------------------------------
void AssignmentDialog::populatePreviewTable(const Assignment& a)
{
    if (!m_previewTable) return;
    m_previewTable->clear();
    m_previewTable->setRowCount(0);
    m_previewTable->setColumnCount(0);

    const auto& bmap = a.boatRowerMap();
    if (bmap.isEmpty()) return;

    QList<int> boatIds = bmap.keys();
    int numCols = boatIds.size();

    int maxRowers = 0;
    for (int bid : boatIds) maxRowers = qMax(maxRowers, bmap[bid].size());

    m_previewTable->setColumnCount(numCols);
    m_previewTable->setRowCount(1 + maxRowers);  // row 0 = meta
    m_previewTable->verticalHeader()->setVisible(false);

    for (int c = 0; c < numCols; ++c) {
        int boatId = boatIds.at(c);
        Boat foundBoat;
        for (const Boat& b : m_boats) if (b.id() == boatId) { foundBoat = b; break; }
        QString header = foundBoat.id() != -1 ? foundBoat.name() : QString("Boat#%1").arg(boatId);
        m_previewTable->setHorizontalHeaderItem(c, new QTableWidgetItem(header));

        // Row 0: metadata
        QString meta;
        if (foundBoat.id() != -1)
            meta = QString("%1|Cap:%2|%3|%4")
                .arg(Boat::boatTypeToString(foundBoat.boatType()))
                .arg(foundBoat.capacity())
                .arg(Boat::steeringTypeToString(foundBoat.steeringType()).left(5))
                .arg(Boat::propulsionTypeToString(foundBoat.propulsionType()).left(5));
        auto* metaItem = new QTableWidgetItem(meta);
        metaItem->setForeground(QColor("#5a7a9a"));
        QFont fi = metaItem->font(); fi.setItalic(true); metaItem->setFont(fi);
        m_previewTable->setItem(0, c, metaItem);
    }

    // Use pickRoles to determine roles for each boat
    for (int c = 0; c < numCols; ++c) {
        int boatId = boatIds.at(c);
        const QList<int>& rowerIds = bmap[boatId];
        Boat foundBoat;
        for (const Boat& b : m_boats) if (b.id() == boatId) { foundBoat = b; break; }

        bool isSteered  = foundBoat.id() != -1 && foundBoat.steeringType() == SteeringType::Steered;
        bool needsRoles = foundBoat.id() == -1 || foundBoat.capacity() > 2;

        int chosenObmann  = -1;
        int chosenSteerer = -1;
        if (needsRoles) {
            auto [ob, st] = pickRoles(rowerIds, m_rowers, isSteered);
            chosenObmann  = ob;
            chosenSteerer = st;
        }

        // Obmann first
        QList<int> ordered;
        if (chosenObmann != -1) ordered << chosenObmann;
        for (int rid : rowerIds) if (rid != chosenObmann) ordered << rid;

        for (int r = 0; r < ordered.size(); ++r) {
            int rid = ordered.at(r);
            QString name;
            for (const Rower& ro : m_rowers) if (ro.id() == rid) { name = ro.name(); break; }
            if (name.isEmpty()) name = QString("Rower#%1").arg(rid);

            QStringList tags;
            if (rid == chosenObmann)
                tags << (rid == chosenSteerer ? "[Obmann][Steering]" : "[Obmann]");
            else if (rid == chosenSteerer)
                tags << "[Steering]";
            // Group tags
            for (const RowingGroup& g : m_groups)
                if (g.rowerIds.contains(rid)) tags << QString("[%1]").arg(g.name);

            if (!tags.isEmpty()) name += "  " + tags.join(" ");

            auto* cell = new QTableWidgetItem(name);
            if (rid == chosenObmann) {
                cell->setForeground(QColor("#f0c060"));
                QFont f = cell->font(); f.setBold(true); cell->setFont(f);
            } else if (rid == chosenSteerer) {
                cell->setForeground(QColor("#60c0f0"));
            }
            m_previewTable->setItem(1 + r, c, cell);
        }
    }

    m_previewTable->resizeColumnsToContents();
    m_previewTable->horizontalHeader()->setStretchLastSection(true);
}

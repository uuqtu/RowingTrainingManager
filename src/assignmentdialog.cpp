#include "assignmentdialog.h"
#include "assignmentgenerator.h"
#include "chartwidgets.h"
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
#include <QScrollArea>
#include <QFrame>
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
    m_tabs->addTab(buildPriorityTab(),  "3 — Options 1");
    m_tabs->addTab(buildOptionsTab(),   "4 — Options 2");
    m_tabs->addTab(buildPreflightTab(), "5 — Pre-flight");
    splitter->addWidget(m_tabs);

    // Right: preview with Text / Table tabs
    auto* previewW = new QWidget;
    auto* previewVL = new QVBoxLayout(previewW);
    previewVL->setContentsMargins(0, 0, 0, 0);
    previewVL->addWidget(new QLabel("Preview  (generated result):"));

    auto* previewTabs = new QTabWidget;
    m_previewTabs = previewTabs;

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

    // Scoring detail tab — populated after each Generate
    m_scoreTabWidget = new QWidget;
    previewTabs->addTab(m_scoreTabWidget, "Scoring");

    // Graphics tab — boat comparison charts
    m_graphicsTabWidget = new QWidget;
    previewTabs->addTab(m_graphicsTabWidget, "Graphics");

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

    // Single save button — label and tooltip adapt to whether a generation
    // result exists. Behaviour is identical to the two previous buttons:
    //   • With result  → ✓ Accept & Save  (same as old Accept & Save)
    //   • Without result → 💾 Save incomplete…  (captures state, no boatRowerMap)
    m_acceptBtn = new QPushButton("💾  Save incomplete…");
    m_acceptBtn->setObjectName("primaryBtn");
    m_acceptBtn->setToolTip(
        "Save this assignment even without a complete generation.\n"
        "It will be marked incomplete (red) in the list and cannot be printed until generated.");

    auto* cancelBtn = new QPushButton("Cancel");
    btnRow->addWidget(m_generateBtn);
    btnRow->addWidget(m_checkBtn);
    btnRow->addStretch();

    // "Preserve groups on save" checkbox — only active after a successful
    // generation. When checked, the saved groups are derived from the result
    // (one group per boat). When unchecked, the user's manually-defined Tab 1
    // groups are preserved as-is. Has no effect on incomplete saves.
    m_preserveGroupsCheck = new QCheckBox("Preserve groups on save");
    m_preserveGroupsCheck->setChecked(true);
    m_preserveGroupsCheck->setEnabled(false);   // enabled only after generation
    m_preserveGroupsCheck->setStyleSheet(
        // Show clearly even when disabled — greyed text but visible box
        "QCheckBox { color: #556677; }"
        "QCheckBox:enabled { color: #8fb4d8; }"
        "QCheckBox::indicator { width: 14px; height: 14px; }"
        "QCheckBox::indicator:checked:disabled { background: #2a4a2a; border: 1px solid #446644; }"
        "QCheckBox::indicator:unchecked:disabled { background: #1a2535; border: 1px solid #3a4a60; }");
    m_preserveGroupsCheck->setToolTip(
        "When checked (default): after Accept & Save, the Groups tab will be\n"
        "populated with the generation result (one group per boat, pinned).\n"
        "This makes the previous session the starting point for the next.\n"
        "When unchecked: the groups you defined manually in Tab 1 are kept as-is.\n"
        "Has no effect when saving an incomplete assignment.\n"
        "Becomes active after a successful generation.");
    btnRow->addWidget(m_preserveGroupsCheck);
    btnRow->addSpacing(8);
    btnRow->addWidget(m_acceptBtn);
    btnRow->addWidget(cancelBtn);
    root->addLayout(btnRow);

    connect(m_generateBtn, &QPushButton::clicked, this, &AssignmentDialog::onGenerate);
    connect(m_checkBtn,    &QPushButton::clicked, this, &AssignmentDialog::onCheck);
    connect(cancelBtn,     &QPushButton::clicked, this, &QDialog::reject);

    // Merged save handler: branch on whether a generation result exists
    connect(m_acceptBtn, &QPushButton::clicked, this, [this]() {
        if (!m_assignment.boatRowerMap().isEmpty()) {
            // Generation was completed — plain accept (stateCapture already ran)
            accept();
        } else {
            // No generation result — save incomplete: capture full dialog state
            QString name = m_nameEdit->text().trimmed();
            if (name.isEmpty())
                name = QString("Assignment %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm"));
            m_assignment.setName(name);
            m_assignment.setCreatedAt(QDateTime::currentDateTime());
            QList<SavedGroup> savedGroups;
            for (const RowingGroup& g : m_groups) {
                SavedGroup sg; sg.name=g.name; sg.rowerIds=g.rowerIds; sg.boatId=g.boatId;
                savedGroups << sg;
            }
            m_assignment.setGroups(savedGroups);
            QList<int> checkedBoats;
            for (int i = 0; i < m_boatList->count(); ++i) {
                auto* it = m_boatList->item(i);
                if (it->checkState() == Qt::Checked) checkedBoats << it->data(Qt::UserRole).toInt();
            }
            m_assignment.setCheckedBoatIds(checkedBoats);
            QList<int> checkedRowers;
            for (int i = 0; i < m_rowerList->count(); ++i) {
                auto* it = m_rowerList->item(i);
                if (it->checkState() == Qt::Checked) checkedRowers << it->data(Qt::UserRole).toInt();
            }
            m_assignment.setCheckedRowerIds(checkedRowers);
            QStringList prio;
            for (int i = 0; i < m_priorityList->count(); ++i) prio << m_priorityList->item(i)->text();
            if (m_trainingCheck && m_trainingCheck->isChecked()) prio << "__training__";
            m_assignment.setPriorityOrder(prio);
            accept();
        }
    });
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
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* inner = new QWidget;
    auto* vl = new QVBoxLayout(inner);
    vl->setSpacing(12);
    vl->setContentsMargins(8,8,8,12);

    // ── Priority order ───────────────────────────────────────────
    auto* sec1 = new QLabel("<b style='color:#8fb4d8;'>Scoring Priority Order</b>");
    vl->addWidget(sec1);
    auto* info = new QLabel(
        "Drag rows or use arrows to set scoring priority. "
        "<b>First = highest weight</b> (4×). Second = 2×. Third = 1×.<br>"
        "The spinboxes below let you override the weight multipliers for this session only — "
        "they reset when the dialog closes and do not change Expert Settings.");
    info->setWordWrap(true);
    info->setStyleSheet("color:#8fb4d8;");
    vl->addWidget(info);

    m_priorityList = new QListWidget;
    m_priorityList->setDragDropMode(QAbstractItemView::InternalMove);
    m_priorityList->setDefaultDropAction(Qt::MoveAction);
    m_priorityList->setMaximumHeight(140);
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

    // ── Weight multiplier overrides ──────────────────────────────
    auto* sep1 = new QFrame; sep1->setFrameShape(QFrame::HLine);
    sep1->setStyleSheet("color:#2a3548;"); vl->addWidget(sep1);

    auto* wHdr = new QLabel("<b style='color:#8fb4d8;'>Weight Multipliers  (session override — resets on close)</b>");
    vl->addWidget(wHdr);
    auto* wDesc = new QLabel(
        "Rank 1 = top of the list. Higher value = that factor dominates more.");
    wDesc->setWordWrap(true);
    wDesc->setStyleSheet("color:#5a7a9a; font-size:11px;");
    vl->addWidget(wDesc);

    m_weightSpins.clear();
    const double defaults[] = {
        m_expertParams.rankWeights[0], m_expertParams.rankWeights[1],
        m_expertParams.rankWeights[2], m_expertParams.rankWeights[3],
        m_expertParams.rankWeights[4]
    };
    auto* grid = new QGridLayout;
    for (int i = 0; i < 5; ++i) {
        auto* lbl = new QLabel(QString("Rank %1 weight:").arg(i+1));
        lbl->setStyleSheet("color:#8090a0; font-size:11px;");
        auto* spin = new QDoubleSpinBox;
        spin->setRange(0.0, 20.0);
        spin->setSingleStep(0.5);
        spin->setDecimals(1);
        spin->setValue(defaults[i]);
        spin->setMaximumWidth(80);
        spin->setToolTip(QString("Override weight for rank %1 (Expert default: %2). "
                                  "Resets on close.").arg(i+1).arg(defaults[i]));
        auto* resetBtn = new QPushButton("↺");
        resetBtn->setMaximumWidth(28);
        resetBtn->setToolTip("Reset to Expert default");
        double def = defaults[i];
        connect(resetBtn, &QPushButton::clicked, spin, [spin, def](){ spin->setValue(def); });
        grid->addWidget(lbl,     i, 0);
        grid->addWidget(spin,    i, 1);
        grid->addWidget(resetBtn,i, 2);
        m_weightSpins.append(spin);
    }
    vl->addLayout(grid);

    // ── Racing boat minimum skill ────────────────────────────────
    auto* sep2 = new QFrame; sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet("color:#2a3548;"); vl->addWidget(sep2);

    auto* racingHdr = new QLabel("<b style='color:#8fb4d8;'>Minimum Skill Level for Racing Boats</b>");
    vl->addWidget(racingHdr);
    auto* racingDesc = new QLabel(
        "Rowers below this skill level are blocked from Racing-type boats in the strict pass (pass 0). "
        "In relaxed passes (2 and 3), the hard block is lifted but a soft penalty still applies. "
        "Lowering this allows less-experienced rowers in Racing boats during relaxed passes — "
        "useful when you cannot form complete teams with the default threshold. "
        "Default: Intermediate (3) — Novice, Beginner, Developing are blocked.");
    racingDesc->setWordWrap(true);
    racingDesc->setStyleSheet("color:#5a7a9a; font-size:11px;");
    vl->addWidget(racingDesc);

    auto* racingRow = new QHBoxLayout;
    racingRow->addWidget(new QLabel("Minimum skill for Racing:"));
    m_racingMinSkillCombo = new QComboBox;
    // Values correspond to SkillLevel integer (1=Novice … 7=Master)
    const struct { const char* label; int val; } skillOpts[] = {
        {"Novice (1) — everyone allowed",            1},
        {"Beginner (2)",                              2},
        {"Developing (3) — block Novice only",       3},
        {"Intermediate (4) — block N/B/Dev [default]",4},
        {"Advanced (5) — block N/B/Dev/Int",          5},
        {"Experienced (6)",                           6},
        {"Master (7) — only Masters allowed",         7},
    };
    for (auto& o : skillOpts)
        m_racingMinSkillCombo->addItem(o.label, o.val);
    // Set current: default racingMinSkill=4 (Intermediate), map to combo index
    {
        int cur = m_expertParams.racingMinSkill > 0 ? m_expertParams.racingMinSkill : 4;
        for (int i = 0; i < m_racingMinSkillCombo->count(); ++i)
            if (m_racingMinSkillCombo->itemData(i).toInt() == cur) {
                m_racingMinSkillCombo->setCurrentIndex(i); break;
            }
    }
    racingRow->addWidget(m_racingMinSkillCombo, 1);
    vl->addLayout(racingRow);

    vl->addStretch();

    auto* outerVL = new QVBoxLayout(w);
    outerVL->setContentsMargins(0,0,0,0);
    scroll->setWidget(inner);
    outerVL->addWidget(scroll);

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

    // Update save button label to reflect whether a result already exists
    if (!m_assignment.boatRowerMap().isEmpty()) {
        m_acceptBtn->setText("\u2713  Accept & Save");
        m_acceptBtn->setToolTip("Save this generated assignment.");
        if (m_preserveGroupsCheck) m_preserveGroupsCheck->setEnabled(true);
    }
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
            if (r.skill() == SkillLevel::Novice || r.skill() == SkillLevel::Beginner || r.skill() == SkillLevel::Developing)
                beginnerCount++;
        int racingCapacity = 0;
        for (const Boat& rb : boats)
            if (rb.boatType() == BoatType::Racing) racingCapacity += rb.capacity();
        int nonBeginnerCount = rowers.size() - beginnerCount;
        if (nonBeginnerCount < racingCapacity)
            issues << QString("Warning: %1 racing seat(s) but only %2 Intermediate+ "
                              "rower(s) — %3 beginner(s) may end up in a racing boat.")
                          .arg(racingCapacity).arg(nonBeginnerCount)
                          .arg(racingCapacity - nonBeginnerCount);
    }

    return issues;
}

void AssignmentDialog::updateGenerateEnabled()
{
    // Guard: may be called before m_generateBtn/m_statusLabel are created
    if (!m_generateBtn || !m_statusLabel) return;

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
    // Apply expert parameters
    // Use session-override weights from Priority tab spinboxes if available,
    // otherwise fall back to ExpertParams
    if (m_weightSpins.size() == 5) {
        p.rankWeights = std::vector<double>{
            m_weightSpins[0]->value(), m_weightSpins[1]->value(),
            m_weightSpins[2]->value(), m_weightSpins[3]->value(),
            m_weightSpins[4]->value()};
    } else {
        p.rankWeights = std::vector<double>{
            m_expertParams.rankWeights[0], m_expertParams.rankWeights[1],
            m_expertParams.rankWeights[2], m_expertParams.rankWeights[3],
            m_expertParams.rankWeights[4]};
    }
    p.whitelistBonus         = m_expertParams.whitelistBonus;
    p.coOccurrenceFactor     = m_expertParams.coOccurrenceFactor;
    p.obmannBonus            = m_expertParams.obmannBonus;
    p.racingBeginnerPenalty  = m_expertParams.racingBeginnerPenalty;
    p.strengthVarianceWeight = m_expertParams.strengthVarianceWeight;
    p.compatSpecialSpecial   = m_expertParams.compatSpecialSpecial;
    p.compatSpecialSelected  = m_expertParams.compatSpecialSelected;
    p.strokeSmallGap1        = m_expertParams.strokeSmallGap1;
    p.strokeSmallGap2        = m_expertParams.strokeSmallGap2;
    p.strokeLargePerGap      = m_expertParams.strokeLargePerGap;
    p.bodySmallGap1          = m_expertParams.bodySmallGap1;
    p.bodySmallGap2          = m_expertParams.bodySmallGap2;
    p.bodyLargePerGap        = m_expertParams.bodyLargePerGap;
    p.grpAttrBonus           = m_expertParams.grpAttrBonus;
    p.valAttrVarianceWeight  = m_expertParams.valAttrVarianceWeight;
    p.fillBoatAttempts       = m_expertParams.fillBoatAttempts;
    p.passAttempts           = m_expertParams.passAttempts;
    p.maximizeLearning       = m_expertParams.maximizeLearning;
    p.ignoreBlacklist        = m_ignoreBlacklistCheck  && m_ignoreBlacklistCheck->isChecked();
    p.ignoreBoatBlacklist    = m_ignoreBoatListsCheck  && m_ignoreBoatListsCheck->isChecked();
    p.ignoreBoatWhitelist    = m_ignoreBoatListsCheck  && m_ignoreBoatListsCheck->isChecked();
    // Racing minimum skill from Options 1 dropdown
    if (m_racingMinSkillCombo)
        p.racingMinSkill = m_racingMinSkillCombo->currentData().toInt();
    else
        p.racingMinSkill = m_expertParams.racingMinSkill;
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

        if (needsRoles && chosenObmann == -1) {
            text += "  *** No Obmann available !\n";
            text += "  *** First rower is Obmann ***\n";
        }

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
        gen.setLogDir(m_expertParams.logDir);
        GeneratorResult result = gen.generate(selectedBoats, selectedRowers, name, priority);
        m_assignment = result.assignment;
        m_assignment.setGroups({});
        m_assignment.setCheckedBoatIds({});
        m_assignment.setCheckedRowerIds({});
        m_assignment.setPriorityOrder({"__crazy__"});
        m_previewEdit->setPlainText(formatPreview(m_assignment));
        populatePreviewTable(m_assignment);
        populateScoreTab(m_assignment, buildPriority());
        populateGraphicsTab(m_assignment, buildPriority());
        m_statusLabel->setText("<font color='#cc6644'>Crazy mode — random distribution applied!</font>");
        m_acceptBtn->setText("\u2713  Accept & Save");
        m_acceptBtn->setToolTip("Save this generated assignment.");
        if (m_preserveGroupsCheck) m_preserveGroupsCheck->setEnabled(true);
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
            if (m_preserveGroupsCheck) m_preserveGroupsCheck->setEnabled(false);
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
        gen.setLogDir(m_expertParams.logDir);
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
            if (m_preserveGroupsCheck) m_preserveGroupsCheck->setEnabled(false);
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
    populateScoreTab(m_assignment, buildPriority());
    populateGraphicsTab(m_assignment, buildPriority());

stateCapture:

    // Always re-read the name from the edit field in case user changed it after Generate
    {
        QString finalName = m_nameEdit->text().trimmed();
        if (!finalName.isEmpty()) m_assignment.setName(finalName);
    }

    // ---- Persist dialog state so double-click restore works exactly ----
    //
    // When "Preserve groups on save" is checked (default) AND a generation
    // result exists: groups are derived from the result (one per boat, pinned).
    // When unchecked OR no result exists: the user's manually-defined Tab 1
    // groups are preserved as-is.
    QList<SavedGroup> savedGroups;
    bool preserveFromResult = !m_assignment.boatRowerMap().isEmpty()
                              && m_preserveGroupsCheck
                              && m_preserveGroupsCheck->isChecked();
    if (preserveFromResult) {
        // Build one SavedGroup per boat from the generation result
        for (auto it = m_assignment.boatRowerMap().constBegin();
             it != m_assignment.boatRowerMap().constEnd(); ++it) {
            int boatId = it.key();
            const QList<int>& rowerIds = it.value();
            if (rowerIds.isEmpty()) continue;

            QString boatName;
            for (const Boat& b : m_boats)
                if (b.id() == boatId) { boatName = b.name(); break; }
            if (boatName.isEmpty())
                boatName = QString("Boat %1").arg(boatId);

            SavedGroup sg;
            sg.name     = boatName;
            sg.boatId   = boatId;
            sg.rowerIds = rowerIds;
            savedGroups << sg;
        }
    } else {
        // Preserve the user's manually-defined dialog groups
        for (const RowingGroup& g : m_groups) {
            SavedGroup sg;
            sg.name     = g.name;
            sg.rowerIds = g.rowerIds;
            sg.boatId   = g.boatId;
            savedGroups << sg;
        }
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
    // Warn if list constraints were bypassed
    QStringList listWarnings;
    if (priority.ignoreBlacklist) {
        // Scan for blacklist violations in the result
        for (auto it = m_assignment.boatRowerMap().constBegin();
             it != m_assignment.boatRowerMap().constEnd(); ++it) {
            const QList<int>& team = it.value();
            for (int i = 0; i < team.size(); ++i) {
                Rower ra; for (const Rower& r:m_rowers) if(r.id()==team[i]){ra=r;break;}
                for (int j = i+1; j < team.size(); ++j) {
                    if (ra.blacklist().contains(team[j]))
                        listWarnings << QString("⚠ Blacklist violated: %1 + %2 in same boat")
                            .arg(ra.name())
                            .arg([&]{QString n;for(const Rower& r:m_rowers)if(r.id()==team[j])n=r.name();return n;}());
                }
            }
        }
    }
    if (!listWarnings.isEmpty())
        statusMsg = QString("<font color='#ffaa44'>Generated — %1 blacklist violation(s): %2</font>")
            .arg(listWarnings.size())
            .arg(listWarnings.join("; ").left(120));
    if (!issues.isEmpty() && listWarnings.isEmpty())
        statusMsg = QString("<font color='#ffaa44'>Generated with %1 warning(s) — see Check for details. Accept to save.</font>")
                        .arg(issues.size());
    m_statusLabel->setText(statusMsg);
    m_acceptBtn->setText("\u2713  Accept & Save");
    m_acceptBtn->setToolTip("Save this generated assignment.");
    if (m_preserveGroupsCheck) m_preserveGroupsCheck->setEnabled(true);
}

// ================================================================
// TAB 4 — OPTIONS
// ================================================================
QWidget* AssignmentDialog::buildOptionsTab()
{
    auto* w = new QWidget;
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* inner = new QWidget;
    auto* vl = new QVBoxLayout(inner);
    vl->setSpacing(12);
    vl->setContentsMargins(8,8,8,12);

    // ── Generation modes ─────────────────────────────────────────
    auto* modesHdr = new QLabel("<b style='color:#8fb4d8;'>Generation Modes</b>");
    vl->addWidget(modesHdr);

    // Training mode
    m_trainingCheck = new QCheckBox("Training mode");
    m_trainingCheck->setStyleSheet("font-weight:600; color:#8fb4d8;");
    vl->addWidget(m_trainingCheck);
    auto* trainingInfo = new QLabel(
        "Ignores skill level and compatibility. Only propulsion ability, "
        "whitelist/blacklist, and attribute proximity are used. "
        "Use for technique sessions where team balance doesn't matter.");
    trainingInfo->setWordWrap(true);
    trainingInfo->setStyleSheet("color:#556677; font-style:italic; font-size:11px;");
    vl->addWidget(trainingInfo);

    // Crazy mode
    m_crazyCheck = new QCheckBox("Crazy mode  (random distribution)");
    m_crazyCheck->setStyleSheet("font-weight:600; color:#cc6644;");
    vl->addWidget(m_crazyCheck);
    auto* crazyInfo = new QLabel(
        "Ignores ALL constraints — blacklists, whitelists, propulsion, skill, "
        "compatibility, groups. Rowers are distributed completely at random.");
    crazyInfo->setWordWrap(true);
    crazyInfo->setStyleSheet("color:#774433; font-style:italic; font-size:11px;");
    vl->addWidget(crazyInfo);

    // ── List overrides ────────────────────────────────────────────
    auto* sep1 = new QFrame; sep1->setFrameShape(QFrame::HLine);
    sep1->setStyleSheet("color:#2a3548;"); vl->addWidget(sep1);

    auto* listHdr = new QLabel("<b style='color:#cc8844;'>List Override  (use with caution)</b>");
    vl->addWidget(listHdr);

    m_ignoreBlacklistCheck = new QCheckBox("Ignore rower blacklists");
    m_ignoreBlacklistCheck->setStyleSheet("color:#cc9944;");
    m_ignoreBlacklistCheck->setChecked(m_expertParams.ignoreBlacklist);
    vl->addWidget(m_ignoreBlacklistCheck);
    auto* blInfo = new QLabel(
        "Normally blacklisted pairs are a hard constraint. "
        "Enabling this removes the hard block so the generator can find a solution "
        "even when blacklist conflicts exist. Violations appear as status bar warnings. Default: OFF.");
    blInfo->setWordWrap(true);
    blInfo->setStyleSheet("color:#774422; font-style:italic; font-size:11px;");
    vl->addWidget(blInfo);

    m_ignoreBoatListsCheck = new QCheckBox("Ignore boat whitelist / boat blacklist");
    m_ignoreBoatListsCheck->setStyleSheet("color:#cc9944;");
    m_ignoreBoatListsCheck->setChecked(m_expertParams.ignoreBoatBlacklist);
    vl->addWidget(m_ignoreBoatListsCheck);
    auto* bwlInfo = new QLabel(
        "Ignores boat whitelist / blacklist constraints so every boat is "
        "available to every rower. Use only when no other solution exists. Default: OFF.");
    bwlInfo->setWordWrap(true);
    bwlInfo->setStyleSheet("color:#774422; font-style:italic; font-size:11px;");
    vl->addWidget(bwlInfo);

    // ── Steering-only people ──────────────────────────────────────
    auto* sep2 = new QFrame; sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet("color:#2a3548;"); vl->addWidget(sep2);

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
    vl->addWidget(soGroup, 1);

    vl->addStretch();

    auto* outerVL = new QVBoxLayout(w);
    outerVL->setContentsMargins(0,0,0,0);
    scroll->setWidget(inner);
    outerVL->addWidget(scroll);

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

// ---------------------------------------------------------------
// Scoring detail tab
// ---------------------------------------------------------------
void AssignmentDialog::populateScoreTab(const Assignment& a, const ScoringPriority& priority)
{
    if (!m_scoreTabWidget) return;

    // Safely clear previous layout and children
    if (QLayout* old = m_scoreTabWidget->layout()) {
        QLayoutItem* item;
        while ((item = old->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        delete old;
    }
    auto* outerVL = new QVBoxLayout(m_scoreTabWidget);
    outerVL->setContentsMargins(0, 0, 0, 0);

    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* inner = new QWidget;
    auto* vl = new QVBoxLayout(inner);
    vl->setContentsMargins(8, 8, 8, 8);
    vl->setSpacing(12);

    AssignmentGenerator gen;
    QList<ScoreDetail> details = gen.computeScoreDetails(a, m_boats, m_rowers, priority);

    auto mkSep = [&]() {
        auto* l = new QFrame; l->setFrameShape(QFrame::HLine);
        l->setStyleSheet("color:#2a3548;"); vl->addWidget(l);
    };

    for (const ScoreDetail& d : details) {
        // Find boat name
        QString boatName = QString("Boat#%1").arg(d.boatId);
        for (const Boat& b : m_boats) if (b.id() == d.boatId) { boatName = b.name(); break; }

        // ── Boat header ──────────────────────────────────────────
        auto* boatHeader = new QLabel(
            QString("<b style='color:#8fb4d8; font-size:13px;'>%1</b>"
                    "  <span style='color:#5a7a9a;'>— Total Score: "
                    "<b style='color:%2;'>%3</b></span>")
                .arg(boatName.toHtmlEscaped())
                .arg(d.totalScore >= 0 ? "#66cc66" : "#cc6666")
                .arg(QString::number(d.totalScore, 'f', 2)));
        boatHeader->setStyleSheet("background:#0d1a2a; padding:4px 8px; border-radius:4px;");
        vl->addWidget(boatHeader);

        // ── Boat-level parameters table ──────────────────────────
        auto* boatTable = new QTableWidget(0, 2);
        boatTable->horizontalHeader()->setVisible(false);
        boatTable->verticalHeader()->setVisible(false);
        boatTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        boatTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        boatTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        boatTable->setSelectionMode(QAbstractItemView::NoSelection);
        boatTable->setStyleSheet(
            "QTableWidget { background:#0a1520; gridline-color:#1a2538; }"
            "QTableWidget::item { padding:3px 6px; }");

        auto addRow = [&](const QString& name, const QString& val, const QString& colour = "") {
            int row = boatTable->rowCount();
            boatTable->insertRow(row);
            auto* nItem = new QTableWidgetItem(name);
            nItem->setForeground(QColor("#8090a0"));
            auto* vItem = new QTableWidgetItem(val);
            if (!colour.isEmpty()) vItem->setForeground(QColor(colour));
            boatTable->setItem(row, 0, nItem);
            boatTable->setItem(row, 1, vItem);
        };

        auto fmtTerm = [](double v, bool invert = false) -> QString {
            return QString("%1%2").arg(v >= 0 ? "+" : "").arg(v, 0, 'f', 2);
        };

        addRow("Mode", d.trainingMode ? "Training" : d.crazyMode ? "Crazy" : "Normal");
        addRow("Weights (wSkill / wCompat / wProp)",
               QString("%1 / %2 / %3")
                   .arg(d.wSkill,0,'f',1).arg(d.wCompat,0,'f',1).arg(d.wProp,0,'f',1));
        if (!d.trainingMode) {
            addRow("  avgSkill",        QString::number(d.avgSkill, 'f', 2));
            addRow("  skillBalance",    fmtTerm(d.skillBalance),     d.skillBalance >= 0 ? "#66cc66" : "#cc8844");
            addRow("  wSkill × (avgSkill + skillBalance)",
                   fmtTerm(d.wSkill * (d.avgSkill + d.skillBalance)));
            addRow("  compatPenalty (raw)",  QString::number(d.compatPenalty, 'f', 2));
            addRow("  wCompat × (−compatPenalty)",
                   fmtTerm(-d.wCompat * d.compatPenalty),  d.compatPenalty > 0 ? "#cc8844":"#66cc66");
        }
        addRow("  avgProp",           QString::number(d.avgProp, 'f', 3));
        addRow("  wProp × avgProp × 3",
               fmtTerm(d.wProp * d.avgProp * 3.0), "#66cc66");
        addRow("  strengthVariance",  QString::number(d.strengthVariance, 'f', 2));
        addRow("  −strengthVar × weight",
               fmtTerm(-d.strengthVariance * priority.strengthVarianceWeight),
               d.strengthVariance > 0 ? "#cc8844" : "#66cc66");
        addRow("  obmannBonus",       fmtTerm(d.obmannBonus), d.obmannBonus > 0 ? "#66cc66":"#8090a0");
        addRow("  racingBegPenalty",  fmtTerm(-d.racingBegPenalty), d.racingBegPenalty > 0 ? "#cc6666":"#8090a0");
        addRow("  strokePenalty",     fmtTerm(-d.strokePenalty),    d.strokePenalty > 0 ? "#cc8844":"#8090a0");
        addRow("  bodyPenalty",       fmtTerm(-d.bodyPenalty),      d.bodyPenalty   > 0 ? "#cc8844":"#8090a0");
        addRow("  grpBonus",          fmtTerm(d.grpBonus),          d.grpBonus      > 0 ? "#66cc66":"#8090a0");
        addRow("  valPenalty",        fmtTerm(-d.valPenalty),       d.valPenalty    > 0 ? "#cc8844":"#8090a0");
        addRow("  coOccurrencePenalty", fmtTerm(-d.coOccurrencePenalty), d.coOccurrencePenalty > 0 ? "#cc8844":"#8090a0");
        addRow("━━ TOTAL SCORE ━━",   fmtTerm(d.totalScore), d.totalScore >= 0 ? "#88ff88":"#ff8888");

        boatTable->resizeRowsToContents();
        boatTable->setFixedHeight(boatTable->rowCount() * 22 + 4);
        vl->addWidget(boatTable);

        // ── Per-rower parameters table ───────────────────────────
        auto* rowerHeader = new QLabel(
            QString("<span style='color:#6a8aaa; font-size:11px;'>Rower details  (%1 rowers)</span>")
                .arg(d.rowers.size()));
        vl->addWidget(rowerHeader);

        // Build columns: Name + one per rower
        int nRowers = d.rowers.size();
        auto* rt = new QTableWidget(0, 1 + nRowers);
        rt->verticalHeader()->setVisible(false);
        rt->setEditTriggers(QAbstractItemView::NoEditTriggers);
        rt->setSelectionMode(QAbstractItemView::NoSelection);
        rt->setStyleSheet(
            "QTableWidget { background:#0a1520; gridline-color:#1a2538; }"
            "QTableWidget::item { padding:3px 6px; }");
        rt->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        for (int col = 1; col <= nRowers; ++col)
            rt->horizontalHeader()->setSectionResizeMode(col, QHeaderView::Stretch);

        // Column headers: row labels + rower names
        rt->setHorizontalHeaderItem(0, new QTableWidgetItem("Parameter"));
        for (int i = 0; i < nRowers; ++i) {
            QString rname = QString("Rower#%1").arg(d.rowers[i].rowerId);
            for (const Rower& r : m_rowers)
                if (r.id() == d.rowers[i].rowerId) { rname = r.name(); break; }
            auto* hdr = new QTableWidgetItem(rname);
            QFont f = hdr->font(); f.setBold(true); hdr->setFont(f);
            rt->setHorizontalHeaderItem(i+1, hdr);
        }

        // Rows of parameters
        struct RowDef { QString label; std::function<QString(const ScoreDetail::RowerDetail&)> fn; };
        const QString
            STROKE_LABELS[] = {"—","Short","Medium","Long"},
            BODY_LABELS[]   = {"—","Small","Medium","Tall"};

        QList<RowDef> rowDefs;
        rowDefs.append(RowDef{"Skill level", [](const ScoreDetail::RowerDetail& r){ return QString::number(r.skillInt); }});
        rowDefs.append(RowDef{"Propulsion match", [](const ScoreDetail::RowerDetail& r){
            return r.propScore==1.0 ? "Exact (1.0)" : r.propScore==0.5 ? "Both (0.5)" : "None (0.0)"; }});
        rowDefs.append(RowDef{"Compat tier", [](const ScoreDetail::RowerDetail& r){ return r.compatTier; }});
        rowDefs.append(RowDef{"Strength", [](const ScoreDetail::RowerDetail& r){
            return r.strength > 0 ? QString::number(r.strength) : QString("—"); }});
        rowDefs.append(RowDef{"Stroke Length", [STROKE_LABELS](const ScoreDetail::RowerDetail& r){
            return r.strokeLength>=0&&r.strokeLength<=3 ? STROKE_LABELS[r.strokeLength] : QString("—"); }});
        rowDefs.append(RowDef{"Body Size", [BODY_LABELS](const ScoreDetail::RowerDetail& r){
            return r.bodySize>=0&&r.bodySize<=3 ? BODY_LABELS[r.bodySize] : QString("—"); }});
        rowDefs.append(RowDef{"Grp Attr 1", [](const ScoreDetail::RowerDetail& r){
            return r.attrGrp1 > 0 ? QString::number(r.attrGrp1) : QString("—"); }});
        rowDefs.append(RowDef{"Grp Attr 2", [](const ScoreDetail::RowerDetail& r){
            return r.attrGrp2 > 0 ? QString::number(r.attrGrp2) : QString("—"); }});
        rowDefs.append(RowDef{"Val Attr 1", [](const ScoreDetail::RowerDetail& r){
            return r.attrVal1 > 0 ? QString::number(r.attrVal1) : QString("—"); }});
        rowDefs.append(RowDef{"Val Attr 2", [](const ScoreDetail::RowerDetail& r){
            return r.attrVal2 > 0 ? QString::number(r.attrVal2) : QString("—"); }});
        rowDefs.append(RowDef{"Is Obmann", [](const ScoreDetail::RowerDetail& r){ return QString(r.isObmann ? "Yes" : "No"); }});
        rowDefs.append(RowDef{"Can Steer", [](const ScoreDetail::RowerDetail& r){ return QString(r.canSteer ? "Yes" : "No"); }});
        rowDefs.append(RowDef{"Whitelist contrib", [](const ScoreDetail::RowerDetail& r){
            return r.whitelistContrib > 0 ? QString("+%1").arg(r.whitelistContrib, 0, 'f', 2) : QString("0"); }});
        rowDefs.append(RowDef{"CoOccur penalty", [](const ScoreDetail::RowerDetail& r){
            return r.coOccContrib > 0 ? QString("â%1").arg(r.coOccContrib, 0, 'f', 2) : QString("0"); }});

        rt->setRowCount(rowDefs.size());
        for (int row2 = 0; row2 < rowDefs.size(); ++row2) {
            auto* lbl = new QTableWidgetItem(rowDefs[row2].label);
            lbl->setForeground(QColor("#8090a0"));
            rt->setItem(row2, 0, lbl);
            for (int col = 0; col < nRowers; ++col) {
                auto* cell = new QTableWidgetItem(rowDefs[row2].fn(d.rowers[col]));
                rt->setItem(row2, col+1, cell);
            }
        }
        rt->resizeRowsToContents();
        rt->setFixedHeight(rt->rowCount() * 22 + 26);
        vl->addWidget(rt);
        mkSep();
    }

    vl->addStretch();
    scroll->setWidget(inner);
    outerVL->addWidget(scroll);

    // Update tab title with boat count
    if (m_previewTabs)
        m_previewTabs->setTabText(2, QString("Scoring (%1 boats)").arg(details.size()));
}

// ---------------------------------------------------------------
// Graphics tab — boat comparison charts
// ---------------------------------------------------------------
void AssignmentDialog::populateGraphicsTab(const Assignment& a, const ScoringPriority& priority)
{
    if (!m_graphicsTabWidget) return;

    // Safely clear previous contents by replacing the widget's layout
    // Delete all child widgets first, then the old layout
    if (QLayout* old = m_graphicsTabWidget->layout()) {
        QLayoutItem* item;
        while ((item = old->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        delete old;
    }

    auto* outerVL = new QVBoxLayout(m_graphicsTabWidget);
    outerVL->setContentsMargins(0,0,0,0);
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* inner = new QWidget;
    auto* vl = new QVBoxLayout(inner);
    vl->setContentsMargins(8,8,8,16);
    vl->setSpacing(16);

    AssignmentGenerator gen;
    QList<ScoreDetail> details = gen.computeScoreDetails(a, m_boats, m_rowers, priority);

    if (details.isEmpty()) {
        vl->addWidget(new QLabel("<i style='color:#556677;'>Generate an assignment first.</i>"));
        vl->addStretch();
        scroll->setWidget(inner);
        outerVL->addWidget(scroll);
        return;
    }

    // Collect boat names
    QList<QString> boatNames;
    for (const ScoreDetail& d : details) {
        QString n = QString("Boat#%1").arg(d.boatId);
        for (const Boat& b : m_boats) if (b.id()==d.boatId) { n=b.name(); break; }
        boatNames << n;
    }

    // Palette
    static const QColor kPalette[] = {
        {100,200,140}, {80,160,220}, {220,160,60}, {200,80,120},
        {160,100,220}, {80,200,200}, {220,140,80}, {140,200,80}
    };
    auto pal = [](int i) { return kPalette[i % 8]; };

    // ── Section label helper ─────────────────────────────────────
    auto mkSec = [&](const QString& t) {
        auto* l = new QLabel(t);
        l->setStyleSheet("color:#8fb4d8; font-weight:700; font-size:12px; "
                         "border-bottom:1px solid #2a3548; padding-bottom:3px;");
        vl->addWidget(l);
    };

    // ── Hint button factory ──────────────────────────────────────
    auto mkHint = [&](const QString& varName, const QString& boatName,
                      double value, double teamAvg) {
        auto* btn = new QPushButton(QString("💡 %1: %2 = %3")
            .arg(boatName).arg(varName).arg(value, 0, 'f', 2));
        btn->setStyleSheet("text-align:left; background:#0d1a2a; color:#f0c060; "
                           "border:1px solid #3a5020; padding:3px 8px; border-radius:3px;");
        btn->setFlat(true);
        QString hint;
        // Build hint based on variable name
        if (varName.contains("Skill")) {
            if (value < teamAvg - 0.3)
                hint = QString("Boat %1 has below-average skill (%2 vs avg %3). "
                    "To improve: reduce the Skill priority weight (w₁–w₅) so that "
                    "less skilled rowers are distributed more evenly. "
                    "Or increase obmannBonus so an experienced Obmann always leads weaker boats.")
                    .arg(boatName).arg(value, 0, 'f', 2).arg(teamAvg, 0, 'f', 2);
            else
                hint = QString("Boat %1 skill (%2) is at or above average. No action needed.")
                    .arg(boatName).arg(value, 0, 'f', 2);
        } else if (varName.contains("Compat")) {
            if (value > 0.5)
                hint = QString("Boat %1 has a compatibility penalty of %2. "
                    "Check if Special/Selected rowers are being placed together. "
                    "Increase compatSpecialSpecial or compatSpecialSelected in Expert Settings. "
                    "Or move one of the Special rowers to a different boat in the Groups tab.")
                    .arg(boatName).arg(value, 0, 'f', 2);
            else
                hint = QString("Compatibility in boat %1 is fine (penalty %2).").arg(boatName).arg(value, 0, 'f', 2);
        } else if (varName.contains("Stroke")) {
            if (value > 0)
                hint = QString("Boat %1 has a stroke length penalty of %2. "
                    "Rowers with mismatched stroke lengths are grouped here. "
                    "Increase strokeSmallGap1/strokeSmallGap2 in Expert Settings to push "
                    "the generator harder toward matching stroke lengths. "
                    "Or manually pin matching rowers to this boat via Groups.")
                    .arg(boatName).arg(value, 0, 'f', 2);
            else
                hint = QString("Stroke length matching in boat %1 is perfect.").arg(boatName);
        } else if (varName.contains("Body")) {
            if (value > 0)
                hint = QString("Boat %1 has a body size penalty of %2. "
                    "Increase bodySmallGap1/bodySmallGap2 in Expert Settings to "
                    "discourage extreme size mismatches. "
                    "For 2-seat boats these are especially critical.")
                    .arg(boatName).arg(value, 0, 'f', 2);
            else
                hint = QString("Body size matching in boat %1 is perfect.").arg(boatName);
        } else if (varName.contains("CoOcc")) {
            if (value > 2)
                hint = QString("Boat %1 has a co-occurrence penalty of %2 — these rowers "
                    "have shared a boat many times before. "
                    "Increase coOccurrenceFactor in Expert Settings to push the generator "
                    "to split up frequent pairings. Or add Blacklist entries to hard-separate them.")
                    .arg(boatName).arg(value, 0, 'f', 2);
            else
                hint = QString("Co-occurrence in boat %1 is acceptable (%2).").arg(boatName).arg(value, 0, 'f', 2);
        } else if (varName.contains("Strength")) {
            if (value > 3)
                hint = QString("Boat %1 has a high strength variance (%2). "
                    "Physical load is unbalanced. Increase strengthVarianceWeight in Expert Settings "
                    "to force the generator to distribute strength more evenly across boats.")
                    .arg(boatName).arg(value, 0, 'f', 2);
            else
                hint = QString("Strength balance in boat %1 is good (variance %2).").arg(boatName).arg(value, 0, 'f', 2);
        } else {
            hint = QString("Variable %1 in boat %2: value %3, team average %4.")
                .arg(varName).arg(boatName).arg(value, 0, 'f', 2).arg(teamAvg, 0, 'f', 2);
        }
        connect(btn, &QPushButton::clicked, btn, [hint, boatName, varName]() {
            QMessageBox mb;
            mb.setWindowTitle(QString("Hint — %1 for %2").arg(varName).arg(boatName));
            mb.setText(hint);
            mb.setIcon(QMessageBox::Information);
            mb.exec();
        });
        return btn;
    };

    // ═══════════════════════════════════════════════════════════
    // SECTION 1: Total Score comparison bar chart
    // ═══════════════════════════════════════════════════════════
    mkSec("Total Score per Boat");
    vl->addWidget(new QLabel(
        "<span style='color:#5a7a9a; font-size:11px;'>"
        "Higher score = generator considers this team more optimal. "
        "Large gaps between boats indicate uneven assignment quality.</span>"));
    {
        auto* chart = new BarChartWidget;
        chart->setTitle("Total Score per Boat");
        chart->setBoatNames(boatNames);
        chart->setFixedHeight(220);
        QList<double> scores;
        // Shift all positive by making the min = 0
        double minS = 0;
        for (const ScoreDetail& d : details) minS = qMin(minS, d.totalScore);
        double avgScore = 0;
        for (const ScoreDetail& d : details) { scores << (d.totalScore - minS); avgScore += d.totalScore; }
        avgScore /= details.size();
        chart->setSeries({{ "Total Score", scores, QColor(100,200,140) }});
        vl->addWidget(chart);

        // Hint buttons
        auto* hintRow = new QHBoxLayout;
        for (int i=0; i<details.size(); ++i) {
            auto* btn = mkHint("Score", boatNames[i], details[i].totalScore, avgScore);
            hintRow->addWidget(btn);
        }
        hintRow->addStretch();
        vl->addLayout(hintRow);
    }

    // ═══════════════════════════════════════════════════════════
    // SECTION 2: Skill & Balance grouped bar chart
    // ═══════════════════════════════════════════════════════════
    mkSec("Skill Level & Balance");
    vl->addWidget(new QLabel(
        "<span style='color:#5a7a9a; font-size:11px;'>"
        "avgSkill: mean skill level (Novice=1 … Master=7). "
        "skillBalance: closeness to a uniform mix (higher = more balanced). "
        "Ideal: similar avgSkill across all boats, and high skillBalance.</span>"));
    {
        auto* chart = new BarChartWidget;
        chart->setTitle("Skill: Average & Balance");
        chart->setBoatNames(boatNames);
        chart->setFixedHeight(220);
        QList<double> avgs, bals;
        double avgAvgSkill=0;
        for (const ScoreDetail& d : details) { avgs << d.avgSkill; avgAvgSkill+=d.avgSkill; }
        avgAvgSkill /= details.size();
        double maxBal = 0;
        for (const ScoreDetail& d : details) maxBal = qMax(maxBal, -d.skillBalance);
        for (const ScoreDetail& d : details)
            bals << (maxBal>0 ? (-d.skillBalance)/maxBal * 4.0 : 0);  // scale to same axis
        chart->setSeries({
            {"avgSkill",      avgs, QColor(100,180,240)},
            {"−skillVariance (×4 scaled)", bals, QColor(80,220,160)}
        });
        vl->addWidget(chart);
        auto* hintRow = new QHBoxLayout;
        for (int i=0; i<details.size(); ++i) {
            auto* btn = mkHint("SkillLevel", boatNames[i], details[i].avgSkill, avgAvgSkill);
            hintRow->addWidget(btn);
        }
        hintRow->addStretch();
        vl->addLayout(hintRow);
    }

    // ═══════════════════════════════════════════════════════════
    // SECTION 3: Penalty breakdown stacked view
    // ═══════════════════════════════════════════════════════════
    mkSec("Penalty Breakdown per Boat");
    vl->addWidget(new QLabel(
        "<span style='color:#5a7a9a; font-size:11px;'>"
        "Each bar shows how much score is lost by each penalty type. "
        "Taller bars = larger penalty = more room to improve. "
        "Click a hint button to learn how to reduce that penalty.</span>"));
    {
        auto* chart = new BarChartWidget;
        chart->setTitle("Penalties by Type");
        chart->setBoatNames(boatNames);
        chart->setFixedHeight(260);
        QList<double> compat, stroke, body, coocc, strength, racing;
        double avgComp=0,avgStr=0,avgBody=0,avgCo=0;
        for (const ScoreDetail& d : details) {
            compat   << d.compatPenalty;
            stroke   << d.strokePenalty;
            body     << d.bodyPenalty;
            coocc    << d.coOccurrencePenalty;
            strength << d.strengthVariance;
            racing   << d.racingBegPenalty;
            avgComp+=d.compatPenalty; avgStr+=d.strokePenalty;
            avgBody+=d.bodyPenalty;   avgCo+=d.coOccurrencePenalty;
        }
        avgComp/=details.size(); avgStr/=details.size();
        avgBody/=details.size(); avgCo/=details.size();
        chart->setSeries({
            {"Compat",    compat,   QColor(220,100,100)},
            {"Stroke",    stroke,   QColor(220,160,60)},
            {"BodySize",  body,     QColor(180,120,220)},
            {"CoOccur",   coocc,    QColor(80,180,220)},
            {"StrengthVar",strength,QColor(120,200,120)},
            {"RacingBeg", racing,   QColor(220,80,80)},
        });
        vl->addWidget(chart);

        // Hint row — one button per boat for the biggest penalty
        auto* hintRow = new QHBoxLayout;
        for (int i=0; i<details.size(); ++i) {
            const ScoreDetail& d = details[i];
            // Find dominant penalty
            double maxPen = 0; QString maxVar = "Score";
            if (d.compatPenalty > maxPen)        { maxPen=d.compatPenalty;       maxVar="Compat"; }
            if (d.strokePenalty > maxPen)         { maxPen=d.strokePenalty;       maxVar="StrokeLength"; }
            if (d.bodyPenalty > maxPen)           { maxPen=d.bodyPenalty;         maxVar="BodySize"; }
            if (d.coOccurrencePenalty > maxPen)   { maxPen=d.coOccurrencePenalty; maxVar="CoOccurrence"; }
            if (d.strengthVariance > maxPen)      { maxPen=d.strengthVariance;    maxVar="Strength"; }
            if (maxPen > 0)
                hintRow->addWidget(mkHint(maxVar, boatNames[i], maxPen, 0));
        }
        hintRow->addStretch();
        vl->addLayout(hintRow);
    }

    // ═══════════════════════════════════════════════════════════
    // SECTION 4: Radar chart — multi-dimensional comparison
    // ═══════════════════════════════════════════════════════════
    mkSec("Boat Profile — Radar Chart");
    vl->addWidget(new QLabel(
        "<span style='color:#5a7a9a; font-size:11px;'>"
        "Normalised values (0=worst, 1=best) for 6 dimensions. "
        "A large, symmetrical polygon means a well-rounded team. "
        "Axis: AvgSkill, PropMatch, NoPenalty, StrengthBal, GrpBonus, ObmannBonus.</span>"));
    {
        // Normalise each axis across all boats
        int n = details.size();
        QList<double> skillV, propV, penV, strV, grpV, obV;
        for (const ScoreDetail& d : details) {
            skillV  << d.avgSkill;
            propV   << d.avgProp;
            penV    << -(d.compatPenalty + d.strokePenalty + d.bodyPenalty
                         + d.coOccurrencePenalty + d.strengthVariance + d.racingBegPenalty);
            strV    << -d.strengthVariance;
            grpV    << d.grpBonus;
            obV     << d.obmannBonus;
        }
        // Normalise each axis to [0,1]
        auto norm = [](QList<double> v) {
            double mn=*std::min_element(v.begin(),v.end());
            double mx=*std::max_element(v.begin(),v.end());
            if (mx-mn < 1e-9) { for (auto& x:v) x=0.5; return v; }
            for (auto& x:v) x=(x-mn)/(mx-mn);
            return v;
        };
        skillV=norm(skillV); propV=norm(propV); penV=norm(penV);
        strV=norm(strV);     grpV=norm(grpV);   obV=norm(obV);

        auto* chart = new RadarChartWidget;
        chart->setTitle("Boat Profile Radar");
        chart->setAxes({"Avg Skill","Propulsion","Low Penalty","Strength Bal","Grp Bonus","Obmann"});
        chart->setFixedHeight(380);
        QList<RadarSeries> series;
        for (int i=0; i<n; ++i)
            series << RadarSeries{boatNames[i],
                {skillV[i],propV[i],penV[i],strV[i],grpV[i],obV[i]}, pal(i)};
        chart->setSeries(series);
        vl->addWidget(chart);
    }

    // ═══════════════════════════════════════════════════════════
    // SECTION 5: Rower attribute heatmap
    // ═══════════════════════════════════════════════════════════
    mkSec("Rower Attributes per Boat — Heatmap");
    vl->addWidget(new QLabel(
        "<span style='color:#5a7a9a; font-size:11px;'>"
        "Each cell shows the team average for that attribute. "
        "Dark = low, bright green = high. "
        "Even column colouring = well-balanced distribution across boats.</span>"));
    {
        // Rows = attributes, Cols = boats
        QList<QString> attrNames = {"Avg Skill","Avg Strength","Avg GrpAttr1",
                                    "Avg GrpAttr2","Avg ValAttr1","Avg ValAttr2",
                                    "Avg StrokeLen","Avg BodySize"};
        QList<QList<double>> vals;
        for (int ai=0; ai<attrNames.size(); ++ai) {
            QList<double> row;
            for (const ScoreDetail& d : details) {
                double sum=0, cnt=0;
                for (const ScoreDetail::RowerDetail& r : d.rowers) {
                    double v=0;
                    switch(ai) {
                    case 0: v=r.skillInt;     cnt+=1; break;
                    case 1: if(r.strength>0){v=r.strength;cnt+=1;} break;
                    case 2: if(r.attrGrp1>0){v=r.attrGrp1;cnt+=1;} break;
                    case 3: if(r.attrGrp2>0){v=r.attrGrp2;cnt+=1;} break;
                    case 4: if(r.attrVal1>0){v=r.attrVal1;cnt+=1;} break;
                    case 5: if(r.attrVal2>0){v=r.attrVal2;cnt+=1;} break;
                    case 6: if(r.strokeLength>0){v=r.strokeLength;cnt+=1;} break;
                    case 7: if(r.bodySize>0){v=r.bodySize;cnt+=1;} break;
                    }
                    sum+=v;
                }
                row << (cnt>0 ? sum/cnt : 0.0);
            }
            vals << row;
        }
        auto* hm = new HeatmapWidget;
        hm->setTitle("Rower Attributes Heatmap");
        hm->setRowLabels(attrNames);
        hm->setColLabels(boatNames);
        hm->setValues(vals);
        hm->setFixedHeight(qMax(200, attrNames.size()*28+60));
        vl->addWidget(hm);
    }

    // ═══════════════════════════════════════════════════════════
    // SECTION 6: Bonus comparison
    // ═══════════════════════════════════════════════════════════
    mkSec("Bonuses per Boat");
    {
        auto* chart = new BarChartWidget;
        chart->setTitle("Bonuses: Whitelist + Group + Obmann");
        chart->setBoatNames(boatNames);
        chart->setFixedHeight(200);
        QList<double> wl, grp, ob;
        for (const ScoreDetail& d : details) {
            double wlSum=0; for (const ScoreDetail::RowerDetail& r:d.rowers) wlSum+=r.whitelistContrib;
            wl  << wlSum;
            grp << d.grpBonus;
            ob  << d.obmannBonus;
        }
        chart->setSeries({
            {"Whitelist",  wl,  QColor(100,200,140)},
            {"Group Attr", grp, QColor(80,160,220)},
            {"Obmann",     ob,  QColor(220,160,60)},
        });
        vl->addWidget(chart);
    }

    vl->addStretch();
    scroll->setWidget(inner);
    outerVL->addWidget(scroll);

    // Update tab title
    if (m_previewTabs) {
        int idx = m_previewTabs->indexOf(m_graphicsTabWidget);
        if (idx >= 0) m_previewTabs->setTabText(idx, "Graphics");
    }
}

// ================================================================
// Pre-flight Tab (Tab 5) — penalty analysis & hints before generation
// ================================================================
QWidget* AssignmentDialog::buildPreflightTab()
{
    auto* w = new QWidget;
    auto* outerVL = new QVBoxLayout(w);
    outerVL->setContentsMargins(0,0,0,0);

    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* inner = new QWidget;
    auto* vl = new QVBoxLayout(inner);
    vl->setContentsMargins(10,10,10,16);
    vl->setSpacing(12);

    auto mkSec = [&](const QString& t) {
        auto* l = new QLabel(t);
        l->setStyleSheet("color:#8fb4d8; font-weight:700; font-size:12px; "
                         "border-bottom:1px solid #2a3548; padding-bottom:3px;");
        vl->addWidget(l);
    };
    auto mkTip = [&](const QString& icon, const QString& title,
                     const QString& body, const QString& fix) {
        auto* g = new QGroupBox;
        g->setStyleSheet("QGroupBox{background:#0d1a2a;border:1px solid #2a3548;"
                         "border-radius:5px;padding:6px;}");
        auto* gl = new QVBoxLayout(g);
        auto* hdr = new QLabel(icon + "  <b style='color:#f0c060;'>" + title + "</b>");
        hdr->setWordWrap(true); gl->addWidget(hdr);
        auto* bd = new QLabel(body);
        bd->setWordWrap(true);
        bd->setStyleSheet("color:#8090a0; font-size:11px;");
        gl->addWidget(bd);
        auto* fx = new QLabel("✅ <b>How to fix:</b> " + fix);
        fx->setWordWrap(true);
        fx->setStyleSheet("color:#60aa60; font-size:11px;");
        gl->addWidget(fx);
        vl->addWidget(g);
    };

    // Header
    auto* header = new QLabel(
        "<b style='color:#8fb4d8;'>Pre-flight Checklist</b><br>"
        "<span style='color:#5a7a9a; font-size:11px;'>"
        "Review these hints before pressing Generate. "
        "They analyse your current rower data and Expert Settings and flag "
        "situations that commonly cause poor results or failed generation.</span>");
    header->setWordWrap(true);
    vl->addWidget(header);

    // ── Section 1: Data completeness warnings ────────────────────
    mkSec("1 — Rower Data Completeness");

    int nRowers = m_rowers.size();
    if (nRowers == 0) {
        vl->addWidget(new QLabel("<i style='color:#ee6644;'>No rowers loaded.</i>"));
    } else {
        // Stroke length fill
        int noStroke=0, noBody=0, noStrength=0, noAge=0;
        for (const Rower& r:m_rowers) {
            if (r.strokeLength()==0) noStroke++;
            if (r.bodySize()==0) noBody++;
            if (r.strength()==0) noStrength++;
            if (r.ageBand()==0) noAge++;
        }
        if (noStroke > nRowers/2)
            mkTip("⚠️","Stroke Length data sparse",
                  QString("%1 of %2 rowers have no Stroke Length set. "
                          "The strokeSmallGap penalties will have little effect since "
                          "rowers without a value (0) are excluded from the calculation.")
                      .arg(noStroke).arg(nRowers),
                  "Open Rowers tab and set Short/Medium/Long for each rower. "
                  "This is especially important for 2-seat boats.");
        if (noBody > nRowers/2)
            mkTip("⚠️","Body Size data sparse",
                  QString("%1 of %2 rowers have no Body Size set. "
                          "Body size penalties are inactive for rowers with value 0.")
                      .arg(noBody).arg(nRowers),
                  "Set Small/Medium/Tall in the Rowers tab for each rower.");
        if (noStrength > nRowers/2)
            mkTip("ℹ️","Strength data sparse",
                  QString("%1 of %2 rowers have no Strength set. "
                          "The strengthVarianceWeight penalty is inactive for these rowers.")
                      .arg(noStrength).arg(nRowers),
                  "Enter strength values (1–10) in the Rowers tab. "
                  "Or set strengthVarianceWeight to 0 in Expert Settings to suppress the warning.");
        if (noAge > nRowers/3)
            mkTip("ℹ️","Age Band data incomplete",
                  QString("%1 of %2 rowers have no Age Band set. "
                          "Obmann and Steerer role selection uses age band — rowers without it "
                          "are treated as mid-range (50).")
                      .arg(noAge).arg(nRowers),
                  "Enter Age Band in the Rowers tab for reliable role rotation.");
        if (noStroke==0 && noBody==0 && noStrength==0)
            vl->addWidget(new QLabel(
                "✅ <span style='color:#60aa60;'>All rower attributes are fully populated. "
                "Excellent data quality for generation.</span>"));
    }

    // ── Section 2: Constraint tension analysis ───────────────────
    mkSec("2 — Constraint Tension");

    // Check for blacklist cliques
    int hardConflicts = 0;
    for (const Rower& a : m_rowers)
        for (int bid : a.blacklist())
            if (std::any_of(m_rowers.begin(), m_rowers.end(),
                [bid](const Rower& b){ return b.id()==bid; }))
                hardConflicts++;
    if (hardConflicts > 0)
        mkTip("🔒","Blacklist entries present",
              QString("%1 rower blacklist entries are active. "
                      "Each pair increases the chance of generation failing in strict pass "
                      "when boats are small or rower count is tight.")
                  .arg(hardConflicts/2),
              "Use Tab 1 (Groups) to pre-pin blacklisted rowers to different boats. "
              "This resolves the constraint before generation begins.");

    // Special/Selected compat check
    int specials=0, selecteds=0;
    for (const Rower& r:m_rowers) {
        if (r.compatibility()==CompatibilityTier::Special) specials++;
        if (r.compatibility()==CompatibilityTier::Selected) selecteds++;
    }
    if (specials > 0 && selecteds > 0)
        mkTip("⚠️","Special + Selected rowers both present",
              QString("You have %1 Special and %2 Selected rowers. "
                      "Every Special+Selected pair is a HARD block in pass 0. "
                      "With %3 rowers and multiple small boats this may force relaxed passes.")
                  .arg(specials).arg(selecteds).arg(nRowers),
              "Pin Special and Selected rowers to separate boats in Tab 1 (Groups) "
              "to guarantee the strict pass succeeds.");

    // ── Section 3: Expert settings review ───────────────────────
    mkSec("3 — Expert Settings Review");

    // These values come from m_expertParams passed in via setExpertParams()
    if (m_expertParams.coOccurrenceFactor > 4.0)
        mkTip("⚠️","Co-occurrence factor is very high",
              QString("coOccurrenceFactor = %1. Pairs who rowed together even 3 times "
                      "pay a penalty of %2, which may override skill and compat preferences. "
                      "In small clubs where everyone rows with everyone, generation may struggle.")
                  .arg(m_expertParams.coOccurrenceFactor)
                  .arg(m_expertParams.coOccurrenceFactor*3, 0, 'f', 1),
              "Reduce coOccurrenceFactor in Expert Settings (try 1.5–2.0), "
              "or add Blacklist entries for pairs that truly should not row together.");

    if (m_expertParams.obmannBonus > 30.0)
        mkTip("ℹ️","Obmann bonus is very large",
              QString("obmannBonus = %1. This strongly dominates all other soft factors. "
                      "Skill balance and compat may be sacrificed to ensure Obmann placement.")
                  .arg(m_expertParams.obmannBonus),
              "If Obmann placement is reliable (all boats get one), "
              "reduce obmannBonus to 20 to give skill/compat more influence.");

    if (m_expertParams.strokeSmallGap2 + m_expertParams.strokeSmallGap1 > 18.0) {
        mkTip("ℹ️","Stroke length penalties are very strict for 2-seat boats",
              QString("strokeSmallGap1=%1, strokeSmallGap2=%2. "
                      "2-seat boats will almost never receive mismatched stroke lengths. "
                      "If stroke length data is incomplete this may leave some 2-seat boats "
                      "under-utilised when no perfect match exists.")
                  .arg(m_expertParams.strokeSmallGap1).arg(m_expertParams.strokeSmallGap2),
              "Reduce strokeSmallGap2 below 12 if you are willing to occasionally "
              "accept Short+Long in a 2-seat boat.");
    }

    if (m_expertParams.maximizeLearning)
        mkTip("🎓","Maximize Learning is ON",
              "The generator will REWARD skill variance within boats — "
              "it actively tries to mix beginners with experienced rowers. "
              "This overrides the normal Skill priority which tries to balance levels.",
              "This is intentional for training sessions. "
              "For competitive assignments, turn off Maximize Learning in Expert Settings.");

    // ── Section 4: Training session tips (5–7 fixed recommendations) ──
    mkSec("4 — Training Session Recommendations");

    vl->addWidget(new QLabel(
        "<span style='color:#5a7a9a; font-size:11px;'>"
        "These are general best-practice recommendations for rowing training sessions. "
        "Apply them by adjusting the settings indicated before generating.</span>"));

    struct Tip { QString icon, title, body, fix; };
    static const Tip tips[] = {
        {"🎓","Mix skill levels for maximum learning",
         "Placing an Advanced/Experienced/Master rower with a Novice/Beginner in the same boat "
         "creates a mentoring dynamic. The beginner observes better technique, receives "
         "live feedback, and has immediate motivation to improve. Research in motor learning "
         "shows that observational learning from a skilled peer accelerates skill acquisition "
         "more effectively than rowing only with same-level peers.",
         "Enable 'Maximize Learning' in Expert Settings, or move Skill to the bottom "
         "of the priority list in Tab 3 and set its weight (Rank 5) to 0.5. "
         "This lets the generator place mixed-level teams freely."},

        {"💪","Balance physical strength across boats",
         "Unequal physical load distribution leads to fatigue asymmetry — one boat's crew "
         "exhausts faster while another underworks. For endurance training this ruins "
         "training stimulus quality. Even strength distribution also improves boat speed "
         "and makes stroke rate easier to maintain.",
         "Enter Strength values (1–10) for all rowers in the Rowers tab. "
         "Increase strengthVarianceWeight to 0.6–1.0 in Expert Settings "
         "to force the generator to balance load more strictly."},

        {"🚣","Match stroke length in 2-seat boats",
         "In sculling pairs (2-seat boats), mismatched stroke lengths cause the blade "
         "of the longer-stroking rower to catch water while the shorter-stroker's blade "
         "is still in drive phase. This creates drag, disrupts balance, and risks injury. "
         "Even a Short+Medium mismatch produces noticeable drag at race pace.",
         "Set Stroke Length (Short/Medium/Long) for all rowers. "
         "Ensure strokeSmallGap2 ≥ 12 and strokeSmallGap1 ≥ 3 in Expert Settings "
         "so 2-seat boats strongly prefer matching lengths."},

        {"🔄","Rotate Obmann and Steerer roles regularly",
         "Rotating leadership roles develops communication skills, situational awareness, "
         "and mutual understanding across the squad. Rowers who only ever follow never "
         "develop the vocal presence needed to lead a boat under race conditions. "
         "Regular rotation also reduces reliance on a single individual.",
         "Ensure obmannOverusePenalty ≥ 3.0 and steerOverusePenalty ≥ 3.0 "
         "in Expert Settings. Check the Statistics tab before generating "
         "to identify overused Obmann/Steerer rowers."},

        {"🤝","Vary pairings across sessions",
         "Rowers who always row with the same partners develop communication shortcuts "
         "that do not transfer to new combinations. Variety in pairings builds "
         "adaptive teamwork and teaches rowers to synchronise with different styles. "
         "This is especially valuable before regatta season when team compositions may change.",
         "Ensure coOccurrenceFactor is set to 1.5–2.5 in Expert Settings. "
         "Review the Co-occurrence table in the Analysis tab to identify "
         "pairs with 5+ shared sessions and consider adding temporary Blacklist entries."},

        {"⚡","Use Training mode for technical sessions",
         "When the goal is technique development rather than competitive pairing, "
         "skill and compatibility constraints become irrelevant. Training mode ignores "
         "both, focusing only on propulsion ability match and attribute proximity. "
         "This allows mixed-level boats designed purely for drill work.",
         "Check 'Training mode' in Tab 3 (Priority) before generating. "
         "Combined with Maximize Learning, this produces maximally diverse boats "
         "optimised for cross-level mentoring."},

        {"📏","Consider body size for single sculls and pairs",
         "In 1x and 2x boats, significantly mismatched body sizes affect the boat's "
         "balance point and optimal seat position. A very small rower paired with a "
         "very tall one requires compromise rigging. For club training this is minor, "
         "but for pairs training targeting technique it adds unnecessary difficulty.",
         "Set Body Size (Small/Medium/Tall) for all rowers. "
         "Increase bodySmallGap2 to 12+ in Expert Settings for strict 2-seat matching, "
         "or keep at 8 (default) for a soft preference."},
    };
    for (auto& t : tips) mkTip(t.icon, t.title, t.body, t.fix);

    vl->addStretch();
    scroll->setWidget(inner);
    outerVL->addWidget(scroll);
    return w;
}

#include "assignmentviewdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QTextEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFont>
#include <QColor>
#include <QApplication>
#include <QClipboard>
#include <QTimer>

AssignmentViewDialog::AssignmentViewDialog(
    const Assignment& assignment,
    const QList<Boat>& boats,
    const QList<Rower>& rowers,
    const QMap<int,QString>& savedRoles,
    QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Assignment: " + assignment.name() + "  [🔒 Locked — view only]");
    resize(820, 640);
    setModal(true);

    auto* vl = new QVBoxLayout(this);
    vl->setContentsMargins(12, 12, 12, 12);
    vl->setSpacing(8);

    // Header info bar
    auto* header = new QLabel(
        QString("<b>%1</b>"
                "<span style='color:#556677;'>  ·  Created %2"
                "  ·  %3 boat(s)"
                "  ·  🔒 Locked</span>")
            .arg(assignment.name().toHtmlEscaped())
            .arg(assignment.createdAt().toString("dd.MM.yyyy  hh:mm"))
            .arg(assignment.boatRowerMap().size())
    );
    header->setStyleSheet("color:#8fb4d8; font-size:13px; padding:4px 0;");
    vl->addWidget(header);

    auto* note = new QLabel(
        "This assignment is locked. Unlock it from the Assignments tab to make changes.");
    note->setStyleSheet("color:#7a4a1a; background:#2a1a0a; padding:4px 8px; "
                        "border-radius:4px; font-style:italic;");
    vl->addWidget(note);

    // Tab widget: Text + Table
    m_tabs = new QTabWidget;

    // ── Text view ────────────────────────────────────────────────
    m_textView = new QTextEdit;
    m_textView->setReadOnly(true);
    QFont mono("Courier New", 11);
    m_textView->setFont(mono);
    buildTextView(assignment, boats, rowers, savedRoles);
    m_tabs->addTab(m_textView, "Text");

    // ── Table view ────────────────────────────────────────────────
    m_tableView = new QTableWidget(0, 0);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->setSelectionMode(QAbstractItemView::NoSelection);
    m_tableView->horizontalHeader()->setDefaultSectionSize(130);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setStyleSheet(
        "QTableWidget { gridline-color: #2a3548; }"
        "QHeaderView::section { background:#1a2535; color:#8fb4d8; "
        "  font-weight:600; padding:4px; border:1px solid #2a3548; }");
    buildTableView(assignment, boats, rowers, savedRoles);
    m_tabs->addTab(m_tableView, "Table");

    // ── Scoring detail view ───────────────────────────────────────
    // Use a default ScoringPriority (no expert params available here,
    // but we show the raw attribute values and all computable terms)
    ScoringPriority defaultPriority;
    m_tabs->addTab(buildScoreView(assignment, boats, rowers, defaultPriority),
                   "Scoring");

    vl->addWidget(m_tabs, 1);

    // Buttons: Copy to Clipboard + Close
    auto* btnRow = new QHBoxLayout;
    auto* copyBtn = new QPushButton("Copy text to clipboard");
    copyBtn->setObjectName("primaryBtn");
    auto* closeBtn = new QPushButton("Close");
    btnRow->addWidget(copyBtn);
    btnRow->addStretch();
    btnRow->addWidget(closeBtn);
    vl->addLayout(btnRow);

    connect(copyBtn,  &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_textView->toPlainText());
        // Brief feedback
        auto* btn = qobject_cast<QPushButton*>(sender());
        if (btn) {
            btn->setText("Copied!");
            QTimer::singleShot(2000, btn, [btn](){ btn->setText("Copy text to clipboard"); });
        }
    });
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void AssignmentViewDialog::buildTextView(
    const Assignment& a,
    const QList<Boat>& boats,
    const QList<Rower>& rowers,
    const QMap<int,QString>& roles)
{
    const int W = 40;
    QString text;
    text += QString("Assignment: %1\n").arg(a.name());
    text += QString("Created:    %1\n").arg(a.createdAt().toString("dd.MM.yyyy  hh:mm:ss"));
    text += QString("Status:     Locked\n");
    text += QString("=").repeated(W) + "\n\n";

    const auto& bmap = a.boatRowerMap();
    for (auto it = bmap.constBegin(); it != bmap.constEnd(); ++it) {
        int boatId = it.key();
        const QList<int>& rowerIds = it.value();

        Boat foundBoat;
        for (const Boat& b : boats) if (b.id() == boatId) { foundBoat = b; break; }

        text += QString("= %1 =\n").arg(
            foundBoat.id() != -1 ? foundBoat.name() : QString("Boat#%1").arg(boatId));
        if (foundBoat.id() != -1)
            text += QString("  [%1 | Cap:%2 | %3 | %4]\n")
                .arg(Boat::boatTypeToString(foundBoat.boatType()))
                .arg(foundBoat.capacity())
                .arg(Boat::steeringTypeToString(foundBoat.steeringType()))
                .arg(Boat::propulsionTypeToString(foundBoat.propulsionType()));

        bool needsRoles = foundBoat.id() == -1 || foundBoat.capacity() > 2;

        // Find Obmann and Steerer from saved roles
        int obmannId = -1, steererId = -1;
        if (needsRoles) {
            for (int rid : rowerIds) {
                QString role = roles.value(rid);
                if (role == "obmann" || role == "obmann_steering") obmannId  = rid;
                if (role == "steering" || role == "obmann_steering") steererId = rid;
            }
        }

        // Obmann first
        if (needsRoles && obmannId == -1 && !rowerIds.isEmpty()) {
            text += "  *** No Obmann available !\n";
            text += "  *** First rower is Obmann ***\n";
        }

        if (obmannId != -1) {
            QString name;
            for (const Rower& r : rowers) if (r.id() == obmannId) { name = r.name(); break; }
            if (name.isEmpty()) name = QString("Rower#%1").arg(obmannId);
            QStringList tags = {"[Obmann]"};
            if (obmannId == steererId) tags << "[Steering]";
            text += QString("  %1  %2\n").arg(name.left(20), -20).arg(tags.join(" "));
        }
        for (int rid : rowerIds) {
            if (rid == obmannId) continue;
            QString name;
            for (const Rower& r : rowers) if (r.id() == rid) { name = r.name(); break; }
            if (name.isEmpty()) name = QString("Rower#%1").arg(rid);
            QString tag = (rid == steererId) ? "  [Steering]" : "";
            text += QString("  %1%2\n").arg(name.left(22), -22).arg(tag);
        }
        text += "\n";
    }
    m_textView->setPlainText(text);
}

void AssignmentViewDialog::buildTableView(
    const Assignment& a,
    const QList<Boat>& boats,
    const QList<Rower>& rowers,
    const QMap<int,QString>& roles)
{
    const auto& bmap = a.boatRowerMap();
    if (bmap.isEmpty()) return;

    QList<int> boatIds = bmap.keys();
    int numCols = boatIds.size();
    int maxRowers = 0;
    for (int bid : boatIds) maxRowers = qMax(maxRowers, bmap[bid].size());

    m_tableView->setColumnCount(numCols);
    m_tableView->setRowCount(1 + maxRowers);   // row 0 = boat metadata

    for (int c = 0; c < numCols; ++c) {
        int boatId = boatIds.at(c);
        const QList<int>& rowerIds = bmap[boatId];

        Boat foundBoat;
        for (const Boat& b : boats) if (b.id() == boatId) { foundBoat = b; break; }

        // Column header
        QString hdr = foundBoat.id() != -1 ? foundBoat.name() : QString("Boat#%1").arg(boatId);
        m_tableView->setHorizontalHeaderItem(c, new QTableWidgetItem(hdr));

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
        m_tableView->setItem(0, c, metaItem);

        // Find roles
        bool needsRoles = foundBoat.id() == -1 || foundBoat.capacity() > 2;
        int obmannId = -1, steererId = -1;
        if (needsRoles) {
            for (int rid : rowerIds) {
                QString role = roles.value(rid);
                if (role == "obmann" || role == "obmann_steering") obmannId  = rid;
                if (role == "steering" || role == "obmann_steering") steererId = rid;
            }
        }

        // Obmann first, then rest
        QList<int> ordered;
        if (obmannId != -1) ordered << obmannId;
        for (int rid : rowerIds) if (rid != obmannId) ordered << rid;

        for (int r = 0; r < ordered.size(); ++r) {
            int rid = ordered.at(r);
            QString name;
            for (const Rower& ro : rowers) if (ro.id() == rid) { name = ro.name(); break; }
            if (name.isEmpty()) name = QString("Rower#%1").arg(rid);

            QString role = roles.value(rid);
            if (role == "obmann")          name += "  [Obmann]";
            else if (role == "steering")   name += "  [Steering]";
            else if (role == "obmann_steering") name += "  [Obmann][Steering]";

            auto* cell = new QTableWidgetItem(name);
            if (role == "obmann" || role == "obmann_steering") {
                cell->setForeground(QColor("#f0c060"));
                QFont f = cell->font(); f.setBold(true); cell->setFont(f);
            } else if (role == "steering") {
                cell->setForeground(QColor("#60c0f0"));
            }
            m_tableView->setItem(1 + r, c, cell);
        }
    }

    m_tableView->resizeColumnsToContents();
    m_tableView->horizontalHeader()->setStretchLastSection(true);
}

#include <QScrollArea>
#include <QFrame>

QWidget* AssignmentViewDialog::buildScoreView(
    const Assignment& a,
    const QList<Boat>& boats,
    const QList<Rower>& rowers,
    const ScoringPriority& priority)
{
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

    AssignmentGenerator gen;
    QList<ScoreDetail> details = gen.computeScoreDetails(a, boats, rowers, priority);

    if (details.isEmpty()) {
        vl->addWidget(new QLabel("<i style='color:#556677;'>No scoring data available.</i>"));
        vl->addStretch();
        scroll->setWidget(inner);
        outerVL->addWidget(scroll);
        return outer;
    }

    auto mkSep = [&]() {
        auto* l = new QFrame;
        l->setFrameShape(QFrame::HLine);
        l->setStyleSheet("color:#2a3548;");
        vl->addWidget(l);
    };

    for (const ScoreDetail& d : details) {
        QString boatName = QString("Boat#%1").arg(d.boatId);
        for (const Boat& b : boats) if (b.id() == d.boatId) { boatName = b.name(); break; }

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

        // ── Boat-level parameters ─────────────────────────────────
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

        auto addRow = [&](const QString& name, const QString& val, const QString& col = "") {
            int r = boatTable->rowCount(); boatTable->insertRow(r);
            auto* ni = new QTableWidgetItem(name); ni->setForeground(QColor("#8090a0"));
            auto* vi = new QTableWidgetItem(val);
            if (!col.isEmpty()) vi->setForeground(QColor(col));
            boatTable->setItem(r, 0, ni); boatTable->setItem(r, 1, vi);
        };
        auto fmt = [](double v) { return QString("%1%2").arg(v>=0?"+":"").arg(v,0,'f',2); };

        addRow("Mode", d.trainingMode ? "Training" : d.crazyMode ? "Crazy" : "Normal");
        addRow("Weights (wSkill / wCompat / wProp)",
               QString("%1 / %2 / %3")
                   .arg(d.wSkill,0,'f',1).arg(d.wCompat,0,'f',1).arg(d.wProp,0,'f',1));
        if (!d.trainingMode) {
            addRow("  avgSkill",     QString::number(d.avgSkill,'f',2));
            addRow("  skillBalance", fmt(d.skillBalance), d.skillBalance>=0?"#66cc66":"#cc8844");
            addRow("  wSkill × (avgSkill + skillBalance)",
                   fmt(d.wSkill*(d.avgSkill+d.skillBalance)));
            addRow("  compatPenalty (raw)", QString::number(d.compatPenalty,'f',2));
            addRow("  wCompat × (−compatPenalty)",
                   fmt(-d.wCompat*d.compatPenalty), d.compatPenalty>0?"#cc8844":"#66cc66");
        }
        addRow("  avgProp",              QString::number(d.avgProp,'f',3));
        addRow("  wProp × avgProp × 3",  fmt(d.wProp*d.avgProp*3.0), "#66cc66");
        addRow("  strengthVariance",     QString::number(d.strengthVariance,'f',2));
        addRow("  −strengthVar × weight", fmt(-d.strengthVariance*priority.strengthVarianceWeight),
               d.strengthVariance>0?"#cc8844":"#8090a0");
        addRow("  obmannBonus",          fmt(d.obmannBonus),        d.obmannBonus>0?"#66cc66":"#8090a0");
        addRow("  racingBegPenalty",     fmt(-d.racingBegPenalty),  d.racingBegPenalty>0?"#cc6666":"#8090a0");
        addRow("  strokePenalty",        fmt(-d.strokePenalty),     d.strokePenalty>0?"#cc8844":"#8090a0");
        addRow("  bodyPenalty",          fmt(-d.bodyPenalty),       d.bodyPenalty>0?"#cc8844":"#8090a0");
        addRow("  grpBonus",             fmt(d.grpBonus),           d.grpBonus>0?"#66cc66":"#8090a0");
        addRow("  valPenalty",           fmt(-d.valPenalty),        d.valPenalty>0?"#cc8844":"#8090a0");
        addRow("  coOccurrencePenalty",  fmt(-d.coOccurrencePenalty),d.coOccurrencePenalty>0?"#cc8844":"#8090a0");
        addRow("━━ TOTAL SCORE ━━",      fmt(d.totalScore),         d.totalScore>=0?"#88ff88":"#ff8888");

        boatTable->resizeRowsToContents();
        boatTable->setFixedHeight(boatTable->rowCount() * 22 + 4);
        vl->addWidget(boatTable);

        // ── Per-rower parameters ──────────────────────────────────
        int nRowers = d.rowers.size();
        if (nRowers == 0) { mkSep(); continue; }

        vl->addWidget(new QLabel(
            QString("<span style='color:#6a8aaa; font-size:11px;'>Rower details  (%1 rowers)</span>")
                .arg(nRowers)));

        auto* rt = new QTableWidget(0, 1 + nRowers);
        rt->verticalHeader()->setVisible(false);
        rt->setEditTriggers(QAbstractItemView::NoEditTriggers);
        rt->setSelectionMode(QAbstractItemView::NoSelection);
        rt->setStyleSheet(
            "QTableWidget { background:#0a1520; gridline-color:#1a2538; }"
            "QTableWidget::item { padding:3px 6px; }");
        rt->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        for (int c = 1; c <= nRowers; ++c)
            rt->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);

        rt->setHorizontalHeaderItem(0, new QTableWidgetItem("Parameter"));
        for (int i = 0; i < nRowers; ++i) {
            QString rname = QString("Rower#%1").arg(d.rowers[i].rowerId);
            for (const Rower& r : rowers)
                if (r.id() == d.rowers[i].rowerId) { rname = r.name(); break; }
            auto* hdr = new QTableWidgetItem(rname);
            QFont f = hdr->font(); f.setBold(true); hdr->setFont(f);
            rt->setHorizontalHeaderItem(i+1, hdr);
        }

        const QString SL[] = {"—","Short","Medium","Long"};
        const QString BS[] = {"—","Small","Medium","Tall"};

        struct Row { QString label; std::function<QString(const ScoreDetail::RowerDetail&)> fn; };
        QList<Row> rows;
        rows.append(Row{"Skill level",   [](const ScoreDetail::RowerDetail& r){ return QString::number(r.skillInt); }});
        rows.append(Row{"Propulsion",    [](const ScoreDetail::RowerDetail& r){
            return r.propScore==1.0?"Exact (1.0)":r.propScore==0.5?"Both (0.5)":"None (0.0)"; }});
        rows.append(Row{"Compat tier",   [](const ScoreDetail::RowerDetail& r){ return r.compatTier; }});
        rows.append(Row{"Strength",      [](const ScoreDetail::RowerDetail& r){
            return r.strength>0?QString::number(r.strength):QString("—"); }});
        rows.append(Row{"Stroke Length", [SL](const ScoreDetail::RowerDetail& r){
            return r.strokeLength>=0&&r.strokeLength<=3?SL[r.strokeLength]:QString("—"); }});
        rows.append(Row{"Body Size",     [BS](const ScoreDetail::RowerDetail& r){
            return r.bodySize>=0&&r.bodySize<=3?BS[r.bodySize]:QString("—"); }});
        rows.append(Row{"Grp Attr 1",    [](const ScoreDetail::RowerDetail& r){
            return r.attrGrp1>0?QString::number(r.attrGrp1):QString("—"); }});
        rows.append(Row{"Grp Attr 2",    [](const ScoreDetail::RowerDetail& r){
            return r.attrGrp2>0?QString::number(r.attrGrp2):QString("—"); }});
        rows.append(Row{"Val Attr 1",    [](const ScoreDetail::RowerDetail& r){
            return r.attrVal1>0?QString::number(r.attrVal1):QString("—"); }});
        rows.append(Row{"Val Attr 2",    [](const ScoreDetail::RowerDetail& r){
            return r.attrVal2>0?QString::number(r.attrVal2):QString("—"); }});
        rows.append(Row{"Is Obmann",     [](const ScoreDetail::RowerDetail& r){ return QString(r.isObmann?"Yes":"No"); }});
        rows.append(Row{"Can Steer",     [](const ScoreDetail::RowerDetail& r){ return QString(r.canSteer?"Yes":"No"); }});
        rows.append(Row{"Whitelist contrib",[](const ScoreDetail::RowerDetail& r){
            return r.whitelistContrib>0?QString("+%1").arg(r.whitelistContrib,0,'f',2):QString("0"); }});
        rows.append(Row{"CoOccur penalty",[](const ScoreDetail::RowerDetail& r){
            return r.coOccContrib>0?QString("\xe2\x88\x92%1").arg(r.coOccContrib,0,'f',2):QString("0"); }});

        rt->setRowCount(rows.size());
        for (int ri = 0; ri < rows.size(); ++ri) {
            auto* lbl = new QTableWidgetItem(rows[ri].label);
            lbl->setForeground(QColor("#8090a0"));
            rt->setItem(ri, 0, lbl);
            for (int ci = 0; ci < nRowers; ++ci)
                rt->setItem(ri, ci+1, new QTableWidgetItem(rows[ri].fn(d.rowers[ci])));
        }
        rt->resizeRowsToContents();
        rt->setFixedHeight(rt->rowCount() * 22 + 26);
        vl->addWidget(rt);
        mkSep();
    }

    vl->addStretch();
    scroll->setWidget(inner);
    outerVL->addWidget(scroll);
    return outer;
}

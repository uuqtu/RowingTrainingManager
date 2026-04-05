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
        if (needsRoles && obmannId == -1 && !rowerIds.isEmpty())
            text += "  *** No Obmann available for this boat! ***\n";
            text += "  *** First rower is Obmann ***\n";

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

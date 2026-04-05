#include "rowerlistsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>

static int checkedCount(QListWidget* lw) {
    int n = 0;
    for (int i = 0; i < lw->count(); ++i)
        if (lw->item(i)->checkState() == Qt::Checked) ++n;
    return n;
}

RowerListsDialog::RowerListsDialog(const Rower& rower,
                                   const QList<Rower>& allRowers,
                                   const QList<Boat>& allBoats,
                                   QWidget* parent)
    : QDialog(parent), m_rower(rower), m_allRowers(allRowers), m_allBoats(allBoats)
{
    setWindowTitle("Lists — " + rower.name());
    setMinimumSize(900, 600);
    resize(1000, 680);
    setModal(true);

    auto* vl = new QVBoxLayout(this);
    auto* header = new QLabel(
        QString("<b>%1</b> — edit rower and boat lists<br>"
                "<span style='color:#5a7a9a; font-size:11px;'>"
                "Tabs show the current count of entries in parentheses.</span>")
            .arg(rower.name().toHtmlEscaped()));
    header->setWordWrap(true);
    header->setStyleSheet("color:#8fb4d8; padding:4px 0 8px 0;");
    vl->addWidget(header);

    m_tabs = new QTabWidget;

    // Tab 1: Rower Whitelist
    auto* rwW = new QWidget;
    auto* rwVL = new QVBoxLayout(rwW);
    auto* rwDesc = new QLabel(
        "Check rowers who should preferably be in the <b>same boat</b> as "
        + rower.name() + ". Each checked pair adds a whitelist bonus to the team score.");
    rwDesc->setWordWrap(true);
    rwDesc->setStyleSheet("color:#8090a0; font-size:11px; padding-bottom:4px;");
    rwVL->addWidget(rwDesc);
    m_rwList = buildRowerList(true);
    rwVL->addWidget(m_rwList);
    m_tabs->addTab(rwW, "Rower Whitelist");

    // Tab 2: Rower Blacklist
    auto* rbW = new QWidget;
    auto* rbVL = new QVBoxLayout(rbW);
    auto* rbDesc = new QLabel(
        "Check rowers who must <b>never</b> be in the same boat as "
        + rower.name() + ". This is a hard constraint — the generator always respects it.");
    rbDesc->setWordWrap(true);
    rbDesc->setStyleSheet("color:#8090a0; font-size:11px; padding-bottom:4px;");
    rbVL->addWidget(rbDesc);
    m_rbList = buildRowerList(false);
    rbVL->addWidget(m_rbList);
    m_tabs->addTab(rbW, "Rower Blacklist");

    // Tab 3: Boat Whitelist
    auto* bwW = new QWidget;
    auto* bwVL = new QVBoxLayout(bwW);
    auto* bwDesc = new QLabel(
        "Check boats that " + rower.name() + " is <b>willing to row in</b>. "
        "If any boats are checked here, this rower will only be assigned to one of them. "
        "Leave all unchecked for no boat preference.");
    bwDesc->setWordWrap(true);
    bwDesc->setStyleSheet("color:#8090a0; font-size:11px; padding-bottom:4px;");
    bwVL->addWidget(bwDesc);
    m_bwList = buildBoatList(true);
    bwVL->addWidget(m_bwList);
    m_tabs->addTab(bwW, "Boat Whitelist");

    // Tab 4: Boat Blacklist
    auto* bbW = new QWidget;
    auto* bbVL = new QVBoxLayout(bbW);
    auto* bbDesc = new QLabel(
        "Check boats that " + rower.name() + " <b>refuses to row in</b>. "
        "This is a hard constraint — the generator will never assign this rower to a checked boat.");
    bbDesc->setWordWrap(true);
    bbDesc->setStyleSheet("color:#8090a0; font-size:11px; padding-bottom:4px;");
    bbVL->addWidget(bbDesc);
    m_bbList = buildBoatList(false);
    bbVL->addWidget(m_bbList);
    m_tabs->addTab(bbW, "Boat Blacklist");

    vl->addWidget(m_tabs, 1);

    // Update tab titles with counts whenever a checkbox changes
    auto updateAll = [this]() { updateTabTitles(); };
    for (QListWidget* lw : {m_rwList, m_rbList, m_bwList, m_bbList})
        connect(lw, &QListWidget::itemChanged, this, updateAll);
    updateTabTitles();

    // Save / Abort buttons
    auto* btns = new QDialogButtonBox;
    auto* saveBtn  = btns->addButton("Save",  QDialogButtonBox::AcceptRole);
    auto* abortBtn = btns->addButton("Abort", QDialogButtonBox::RejectRole);
    Q_UNUSED(saveBtn); Q_UNUSED(abortBtn);
    vl->addWidget(btns);
    connect(btns, &QDialogButtonBox::accepted, this, [this]() {
        // Collect results from all four lists
        QList<int> rw, rb, bw, bb;
        for (int i = 0; i < m_rwList->count(); ++i)
            if (m_rwList->item(i)->checkState() == Qt::Checked)
                rw << m_rwList->item(i)->data(Qt::UserRole).toInt();
        for (int i = 0; i < m_rbList->count(); ++i)
            if (m_rbList->item(i)->checkState() == Qt::Checked)
                rb << m_rbList->item(i)->data(Qt::UserRole).toInt();
        for (int i = 0; i < m_bwList->count(); ++i)
            if (m_bwList->item(i)->checkState() == Qt::Checked)
                bw << m_bwList->item(i)->data(Qt::UserRole).toInt();
        for (int i = 0; i < m_bbList->count(); ++i)
            if (m_bbList->item(i)->checkState() == Qt::Checked)
                bb << m_bbList->item(i)->data(Qt::UserRole).toInt();
        m_rower.setWhitelist(rw);
        m_rower.setBlacklist(rb);
        m_rower.setBoatWhitelist(bw);
        m_rower.setBoatBlacklist(bb);
        accept();
    });
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QListWidget* RowerListsDialog::buildRowerList(bool whitelist) {
    auto* lw = new QListWidget;
    lw->setAlternatingRowColors(true);
    const QList<int>& current = whitelist ? m_rower.whitelist() : m_rower.blacklist();
    for (const Rower& r : m_allRowers) {
        if (r.id() == m_rower.id()) continue;  // skip self
        auto* item = new QListWidgetItem(r.name());
        item->setData(Qt::UserRole, r.id());
        item->setCheckState(current.contains(r.id()) ? Qt::Checked : Qt::Unchecked);
        lw->addItem(item);
    }
    return lw;
}

QListWidget* RowerListsDialog::buildBoatList(bool whitelist) {
    auto* lw = new QListWidget;
    lw->setAlternatingRowColors(true);
    const QList<int>& current = whitelist ? m_rower.boatWhitelist() : m_rower.boatBlacklist();
    for (const Boat& b : m_allBoats) {
        QString label = QString("%1  [Cap:%2 | %3 | %4]")
            .arg(b.name()).arg(b.capacity())
            .arg(Boat::steeringTypeToString(b.steeringType()))
            .arg(Boat::propulsionTypeToString(b.propulsionType()));
        auto* item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, b.id());
        item->setCheckState(current.contains(b.id()) ? Qt::Checked : Qt::Unchecked);
        lw->addItem(item);
    }
    return lw;
}

void RowerListsDialog::updateTabTitles() {
    m_tabs->setTabText(0, QString("Rower Whitelist (%1)").arg(checkedCount(m_rwList)));
    m_tabs->setTabText(1, QString("Rower Blacklist (%1)").arg(checkedCount(m_rbList)));
    m_tabs->setTabText(2, QString("Boat Whitelist (%1)").arg(checkedCount(m_bwList)));
    m_tabs->setTabText(3, QString("Boat Blacklist (%1)").arg(checkedCount(m_bbList)));
}

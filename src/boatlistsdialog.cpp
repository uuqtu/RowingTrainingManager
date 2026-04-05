#include "boatlistsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>

static int checkedCount(QListWidget* lw) {
    int n=0;
    for (int i=0;i<lw->count();++i)
        if (lw->item(i)->checkState()==Qt::Checked) ++n;
    return n;
}

BoatListsDialog::BoatListsDialog(const Boat& boat,
                                  const QList<Rower>& allRowers,
                                  QWidget* parent)
    : QDialog(parent), m_boat(boat), m_rowers(allRowers)
{
    setWindowTitle(QString("Lists — %1").arg(boat.name()));
    setMinimumSize(900, 560);
    resize(1000, 640);
    setModal(true);

    auto* vl = new QVBoxLayout(this);
    auto* header = new QLabel(
        QString("<b>%1</b>  [Cap: %2 | %3 | %4]<br>"
                "<span style='color:#5a7a9a; font-size:11px;'>"
                "Manage which rowers are allowed or excluded from this boat. "
                "These settings are stored on each rower and have the same effect as "
                "editing the rower's Boat Whitelist or Boat Blacklist directly.</span>")
            .arg(boat.name().toHtmlEscaped())
            .arg(boat.capacity())
            .arg(Boat::steeringTypeToString(boat.steeringType()))
            .arg(Boat::propulsionTypeToString(boat.propulsionType())));
    header->setWordWrap(true);
    header->setStyleSheet("color:#8fb4d8; padding:4px 0 8px 0;");
    vl->addWidget(header);

    m_tabs = new QTabWidget;

    // Tab 1: Allowed rowers (boat whitelist from rower side)
    auto* awW = new QWidget;
    auto* awVL = new QVBoxLayout(awW);
    auto* awDesc = new QLabel(
        "<b>Allowed rowers (Boat Whitelist)</b><br>"
        "Checking a rower here adds this boat to that rower's Boat Whitelist. "
        "A rower with a non-empty Boat Whitelist will <b>only</b> be assigned to boats on their list. "
        "<i>Leave all unchecked to allow this boat for all rowers by default.</i>");
    awDesc->setWordWrap(true);
    awDesc->setStyleSheet("color:#8090a0; font-size:11px; padding-bottom:4px;");
    awVL->addWidget(awDesc);
    m_allowList = buildAllowedList();
    awVL->addWidget(m_allowList);
    m_tabs->addTab(awW, "Allowed Rowers");

    // Tab 2: Excluded rowers (boat blacklist from rower side)
    auto* exW = new QWidget;
    auto* exVL = new QVBoxLayout(exW);
    auto* exDesc = new QLabel(
        "<b>Excluded rowers (Boat Blacklist)</b><br>"
        "Checking a rower here adds this boat to that rower's Boat Blacklist. "
        "The generator will <b>never</b> assign a checked rower to this boat. "
        "This is a hard constraint.");
    exDesc->setWordWrap(true);
    exDesc->setStyleSheet("color:#8090a0; font-size:11px; padding-bottom:4px;");
    exVL->addWidget(exDesc);
    m_excludeList = buildExcludedList();
    exVL->addWidget(m_excludeList);
    m_tabs->addTab(exW, "Excluded Rowers");

    vl->addWidget(m_tabs, 1);

    for (QListWidget* lw : {m_allowList, m_excludeList})
        connect(lw, &QListWidget::itemChanged, this, &BoatListsDialog::updateTabTitles);
    updateTabTitles();

    auto* btns = new QDialogButtonBox;
    btns->addButton("Save",  QDialogButtonBox::AcceptRole);
    btns->addButton("Abort", QDialogButtonBox::RejectRole);
    vl->addWidget(btns);

    connect(btns, &QDialogButtonBox::accepted, this, [this]() {
        int boatId = m_boat.id();
        // Apply changes: for each rower, add/remove this boat from their whitelist/blacklist
        for (int i=0; i<m_allowList->count(); ++i) {
            auto* item = m_allowList->item(i);
            int rowerId = item->data(Qt::UserRole).toInt();
            for (Rower& r : m_rowers) {
                if (r.id() != rowerId) continue;
                QList<int> bwl = r.boatWhitelist();
                if (item->checkState()==Qt::Checked && !bwl.contains(boatId))
                    bwl << boatId;
                else if (item->checkState()==Qt::Unchecked)
                    bwl.removeAll(boatId);
                r.setBoatWhitelist(bwl);
                break;
            }
        }
        for (int i=0; i<m_excludeList->count(); ++i) {
            auto* item = m_excludeList->item(i);
            int rowerId = item->data(Qt::UserRole).toInt();
            for (Rower& r : m_rowers) {
                if (r.id() != rowerId) continue;
                QList<int> bbl = r.boatBlacklist();
                if (item->checkState()==Qt::Checked && !bbl.contains(boatId))
                    bbl << boatId;
                else if (item->checkState()==Qt::Unchecked)
                    bbl.removeAll(boatId);
                r.setBoatBlacklist(bbl);
                break;
            }
        }
        accept();
    });
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QListWidget* BoatListsDialog::buildAllowedList() {
    auto* lw = new QListWidget;
    lw->setSortingEnabled(true);
    lw->setAlternatingRowColors(true);
    int boatId = m_boat.id();
    for (const Rower& r : m_rowers) {
        QString label = QString("%1  [%2 | %3]")
            .arg(r.name())
            .arg(Rower::skillToString(r.skill()))
            .arg(Boat::propulsionTypeToString(r.propulsionAbility()));
        auto* item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, r.id());
        item->setCheckState(r.boatWhitelist().contains(boatId) ? Qt::Checked : Qt::Unchecked);
        lw->addItem(item);
    }
    return lw;
}

QListWidget* BoatListsDialog::buildExcludedList() {
    auto* lw = new QListWidget;
    lw->setSortingEnabled(true);
    lw->setAlternatingRowColors(true);
    int boatId = m_boat.id();
    for (const Rower& r : m_rowers) {
        QString label = QString("%1  [%2 | %3]")
            .arg(r.name())
            .arg(Rower::skillToString(r.skill()))
            .arg(Boat::propulsionTypeToString(r.propulsionAbility()));
        auto* item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, r.id());
        item->setCheckState(r.boatBlacklist().contains(boatId) ? Qt::Checked : Qt::Unchecked);
        lw->addItem(item);
    }
    return lw;
}

void BoatListsDialog::updateTabTitles() {
    m_tabs->setTabText(0, QString("Allowed Rowers (%1)").arg(checkedCount(m_allowList)));
    m_tabs->setTabText(1, QString("Excluded Rowers (%1)").arg(checkedCount(m_excludeList)));
}

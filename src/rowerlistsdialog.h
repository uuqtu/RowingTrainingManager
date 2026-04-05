#pragma once
#include <QDialog>
#include "rower.h"
#include "boat.h"

class QTabWidget;
class QListWidget;

// Unified dialog for editing all four lists of a rower:
// Rower Whitelist, Rower Blacklist, Boat Whitelist, Boat Blacklist.
class RowerListsDialog : public QDialog {
    Q_OBJECT
public:
    explicit RowerListsDialog(const Rower& rower,
                               const QList<Rower>& allRowers,
                               const QList<Boat>& allBoats,
                               QWidget* parent = nullptr);

    // Call after accepted() to get the modified rower
    Rower result() const { return m_rower; }

private:
    QListWidget* buildRowerList(bool whitelist);
    QListWidget* buildBoatList(bool whitelist);
    void updateTabTitles();

    Rower          m_rower;
    QList<Rower>   m_allRowers;
    QList<Boat>    m_allBoats;
    QTabWidget*    m_tabs = nullptr;
    QListWidget*   m_rwList = nullptr;  // rower whitelist
    QListWidget*   m_rbList = nullptr;  // rower blacklist
    QListWidget*   m_bwList = nullptr;  // boat whitelist
    QListWidget*   m_bbList = nullptr;  // boat blacklist
};

#pragma once
#include <QDialog>
#include "boat.h"
#include "rower.h"

class QTabWidget;
class QListWidget;

// Boat-perspective lists dialog.
// Shows which rowers allow / exclude this boat.
// Toggling a checkbox updates the rower's boatWhitelist / boatBlacklist.
//
// boatWhitelist on rower: rower ONLY rows in whitelisted boats (if non-empty).
//   From the boat's view: "Allowed rowers" (active = rower has this boat in their whitelist)
// boatBlacklist on rower: rower NEVER rows in blacklisted boats.
//   From the boat's view: "Excluded rowers" (active = rower has this boat in their blacklist)
//
// Opposite logic from RowerListsDialog:
//   RowerListsDialog "boat whitelist" tab = rower chooses which boats it wants.
//   BoatListsDialog  "allowed rowers" tab = boat chooses which rowers can board.

class BoatListsDialog : public QDialog {
    Q_OBJECT
public:
    explicit BoatListsDialog(const Boat& boat,
                             const QList<Rower>& allRowers,
                             QWidget* parent = nullptr);

    // Returns the updated rower list (all rowers, with modified whitelist/blacklist)
    QList<Rower> updatedRowers() const { return m_rowers; }

private:
    void updateTabTitles();
    QListWidget* buildAllowedList();
    QListWidget* buildExcludedList();

    Boat         m_boat;
    QList<Rower> m_rowers;
    QTabWidget*  m_tabs      = nullptr;
    QListWidget* m_allowList = nullptr;  // rowers with this boat in boatWhitelist
    QListWidget* m_excludeList = nullptr; // rowers with this boat in boatBlacklist
};

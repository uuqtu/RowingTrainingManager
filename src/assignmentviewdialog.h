#pragma once
#include <QDialog>
#include <QList>
#include "assignment.h"
#include "boat.h"
#include "rower.h"

class QTextEdit;
class QTableWidget;
class QTabWidget;
class QLabel;

// Read-only display window for a saved assignment.
// Used for locked assignments where the edit dialog is blocked.
class AssignmentViewDialog : public QDialog {
    Q_OBJECT
public:
    explicit AssignmentViewDialog(
        const Assignment& assignment,
        const QList<Boat>& boats,
        const QList<Rower>& rowers,
        const QMap<int,QString>& savedRoles,   // rowerId -> "obmann"|"steering"|"obmann_steering"
        QWidget* parent = nullptr);

private:
    void buildTextView(const Assignment& a,
                       const QList<Boat>& boats,
                       const QList<Rower>& rowers,
                       const QMap<int,QString>& roles);
    void buildTableView(const Assignment& a,
                        const QList<Boat>& boats,
                        const QList<Rower>& rowers,
                        const QMap<int,QString>& roles);

    QTextEdit*    m_textView  = nullptr;
    QTableWidget* m_tableView = nullptr;
    QTabWidget*   m_tabs      = nullptr;
};

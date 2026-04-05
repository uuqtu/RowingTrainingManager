#pragma once
#include <QDialog>
#include <QList>
#include <QMap>
#include "assignment.h"
#include "boat.h"
#include "rower.h"
#include "assignmentgenerator.h"

class QTextEdit;
class QTableWidget;
class QTabWidget;
class QLabel;
class QWidget;

class AssignmentViewDialog : public QDialog {
    Q_OBJECT
public:
    explicit AssignmentViewDialog(
        const Assignment& assignment,
        const QList<Boat>& boats,
        const QList<Rower>& rowers,
        const QMap<int,QString>& savedRoles,
        QWidget* parent = nullptr);

private:
    void buildTextView(const Assignment& a, const QList<Boat>& boats,
                       const QList<Rower>& rowers, const QMap<int,QString>& roles);
    void buildTableView(const Assignment& a, const QList<Boat>& boats,
                        const QList<Rower>& rowers, const QMap<int,QString>& roles);
    QWidget* buildScoreView(const Assignment& a, const QList<Boat>& boats,
                            const QList<Rower>& rowers, const ScoringPriority& priority);

    QTextEdit*    m_textView  = nullptr;
    QTableWidget* m_tableView = nullptr;
    QTabWidget*   m_tabs      = nullptr;
};

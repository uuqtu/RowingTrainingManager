#pragma once
#include <QDialog>
#include <QString>
#include <QList>

class QListWidget;
class QPushButton;
class QLabel;

struct TeamEntry {
    QString name;
    QString dbPath;
};

class TeamSelectDialog : public QDialog {
    Q_OBJECT
public:
    explicit TeamSelectDialog(QWidget* parent = nullptr);

    QString selectedDbPath() const { return m_selectedPath; }
    QString selectedTeamName() const { return m_selectedName; }

    // Persist/load team list from a JSON-like config file
    static QList<TeamEntry> loadTeams();
    static void saveTeams(const QList<TeamEntry>& teams);
    static QString configPath();

private slots:
    void onAdd();
    void onRemove();
    void onSelect();

private:
    void refresh();
    QListWidget*  m_list   = nullptr;
    QPushButton*  m_selBtn = nullptr;
    QString       m_selectedPath;
    QString       m_selectedName;
    QList<TeamEntry> m_teams;
};

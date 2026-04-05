#include "teamselectdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QApplication>

// -----------------------------------------------------------------------
// Config file: one line per team  "TeamName|/path/to/db.sqlite"
// -----------------------------------------------------------------------
QString TeamSelectDialog::configPath() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/teams.conf";
}

QList<TeamEntry> TeamSelectDialog::loadTeams() {
    QList<TeamEntry> list;
    QFile f(configPath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return list;
    QTextStream in(&f);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;
        int sep = line.indexOf('|');
        if (sep < 0) continue;
        TeamEntry e;
        e.name   = line.left(sep).trimmed();
        e.dbPath = line.mid(sep+1).trimmed();
        if (!e.name.isEmpty() && !e.dbPath.isEmpty())
            list << e;
    }
    return list;
}

void TeamSelectDialog::saveTeams(const QList<TeamEntry>& teams) {
    QFile f(configPath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) return;
    QTextStream out(&f);
    out << "# RowingManager team list — Name|DatabasePath\n";
    for (const TeamEntry& e : teams)
        out << e.name << "|" << e.dbPath << "\n";
}

// -----------------------------------------------------------------------
TeamSelectDialog::TeamSelectDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("RowingManager — Select Team");
    setModal(true);
    resize(560, 380);
    setStyleSheet(
        "QDialog { background:#0a1520; color:#c8d8e8; }"
        "QLabel { color:#8fb4d8; }"
        "QListWidget { background:#0d1a2a; border:1px solid #2a3548; "
        "              color:#c8d8e8; font-size:13px; }"
        "QListWidget::item:selected { background:#1a3548; color:#ffffff; }"
        "QPushButton { background:#1a2535; color:#8fb4d8; border:1px solid #2a3548; "
        "              padding:5px 14px; border-radius:3px; }"
        "QPushButton:hover { background:#253545; }"
        "QPushButton#selectBtn { background:#1a3a1a; color:#88ee88; "
        "                        border:1px solid #3a5a3a; font-weight:700; }");

    auto* vl = new QVBoxLayout(this);
    vl->setSpacing(12);
    vl->setContentsMargins(16,16,16,16);

    auto* hdr = new QLabel(
        "<b style='color:#8fb4d8; font-size:15px;'>Select Team / Database</b><br>"
        "<span style='color:#5a7a9a; font-size:11px;'>"
        "Each team has its own database with separate rowers, boats, and assignments.</span>");
    hdr->setWordWrap(true);
    vl->addWidget(hdr);

    m_list = new QListWidget;
    m_list->setAlternatingRowColors(true);
    vl->addWidget(m_list, 1);

    auto* btnRow = new QHBoxLayout;
    auto* addBtn = new QPushButton("＋  New Team");
    auto* remBtn = new QPushButton("✕  Remove");
    m_selBtn = new QPushButton("▶  Open Selected Team");
    m_selBtn->setObjectName("selectBtn");
    btnRow->addWidget(addBtn);
    btnRow->addWidget(remBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_selBtn);
    vl->addLayout(btnRow);

    auto* note = new QLabel(
        "<span style='color:#445566; font-size:10px;'>"
        "Databases are stored wherever you place them. "
        "The team list is saved in: " + configPath() + "</span>");
    note->setWordWrap(true);
    vl->addWidget(note);

    m_teams = loadTeams();

    // If no teams exist, seed a default
    if (m_teams.isEmpty()) {
        TeamEntry def;
        def.name   = "My Club";
        QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dir);
        def.dbPath = dir + "/MyClub.db";
        m_teams << def;
        saveTeams(m_teams);
    }

    refresh();

    connect(addBtn,   &QPushButton::clicked, this, &TeamSelectDialog::onAdd);
    connect(remBtn,   &QPushButton::clicked, this, &TeamSelectDialog::onRemove);
    connect(m_selBtn, &QPushButton::clicked, this, &TeamSelectDialog::onSelect);
    connect(m_list,   &QListWidget::itemDoubleClicked, this, &TeamSelectDialog::onSelect);
    connect(m_list,   &QListWidget::currentRowChanged, this,
        [this](int r){ m_selBtn->setEnabled(r >= 0); });
    m_selBtn->setEnabled(!m_teams.isEmpty());
    if (!m_teams.isEmpty()) m_list->setCurrentRow(0);
}

void TeamSelectDialog::refresh() {
    m_list->clear();
    for (const TeamEntry& e : m_teams) {
        auto* item = new QListWidgetItem(
            QString("  %1\n  %2").arg(e.name).arg(e.dbPath));
        item->setSizeHint({0, 44});
        m_list->addItem(item);
    }
}

void TeamSelectDialog::onAdd() {
    bool ok = false;
    QString name = QInputDialog::getText(this, "New Team", "Team name:", QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    QString path = QFileDialog::getSaveFileName(
        this, "Choose database file location",
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
            + "/" + name.trimmed().replace(' ','_') + ".db",
        "Database files (*.db *.sqlite);;All files (*)");
    if (path.isEmpty()) return;

    TeamEntry e;
    e.name   = name.trimmed();
    e.dbPath = path;
    m_teams << e;
    saveTeams(m_teams);
    refresh();
    m_list->setCurrentRow(m_teams.size() - 1);
}

void TeamSelectDialog::onRemove() {
    int row = m_list->currentRow();
    if (row < 0 || row >= m_teams.size()) return;
    auto r = QMessageBox::question(this, "Remove Team",
        QString("Remove team \"%1\" from the list?\n(The database file itself will not be deleted.)")
            .arg(m_teams[row].name));
    if (r != QMessageBox::Yes) return;
    m_teams.removeAt(row);
    saveTeams(m_teams);
    refresh();
}

void TeamSelectDialog::onSelect() {
    int row = m_list->currentRow();
    if (row < 0 || row >= m_teams.size()) return;
    m_selectedPath = m_teams[row].dbPath;
    m_selectedName = m_teams[row].name;
    accept();
}

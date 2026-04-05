#include <QApplication>
#include "mainwindow.h"
#include "teamselectdialog.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Rowing Team Manager");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("RowingClub");

    // Show team/database selection on startup
    TeamSelectDialog sel;
    if (sel.exec() != QDialog::Accepted)
        return 0;   // user closed without selecting

    MainWindow window;
    if (!window.openDatabase(sel.selectedDbPath(), sel.selectedTeamName())) {
        return 1;
    }
    window.show();
    return app.exec();
}

#include <QApplication>
#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Rowing Team Manager");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("RowingClub");

    MainWindow window;
    window.show();
    return app.exec();
}

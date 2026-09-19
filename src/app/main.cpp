// Command hash-gui is the Qt entry point for the hashing tool GUI.
#include "app/mainwindow.h"
#include "app/theme.h"
#include "core/types.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication application(argc, argv);
    hash_ui::applyDarkTheme(application);

    qRegisterMetaType<hash_core::Progress>("hash_core::Progress");
    qRegisterMetaType<QVector<hash_core::FileResult>>("QVector<hash_core::FileResult>");

    MainWindow window;
    window.show();
    return application.exec();
}

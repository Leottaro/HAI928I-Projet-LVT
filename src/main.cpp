#include "mainWindow.hpp"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QScreen>
#include <QSurfaceFormat>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("Projet 3D");
    QCoreApplication::setOrganizationName("M2 IMAGINE");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);
    format.setVersion(3, 2);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);

    MainWindow main_window;
    main_window.resize(main_window.sizeHint());
    QScreen *screen = app.primaryScreen();
    QRect screenGeometry = screen->geometry();
    int height = screenGeometry.height();
    int width = screenGeometry.width();
    int desktopArea = width * height;
    int widgetArea = main_window.width() * main_window.height();
    if (((float) widgetArea / (float) desktopArea) < 0.75f)
        main_window.show();
    else
        main_window.showMaximized();

    return app.exec();
}

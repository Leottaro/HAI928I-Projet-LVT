#include "mainWindow.hpp"

#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtGui/QScreen>
#include <QtGui/QSurfaceFormat>
#include <QtWidgets/QApplication>

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
    main_window.show();

    return app.exec();
}

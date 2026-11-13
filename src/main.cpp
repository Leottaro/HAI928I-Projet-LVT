#include "mainWindow.hpp"

#include <QApplication>

int main(int argc, char *argv[]) {
    QCoreApplication::setApplicationName("Projet 3D");
    QCoreApplication::setOrganizationName("M2 IMAGINE");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);
    format.setVersion(4, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);

    MainWindow main_window;
    main_window.resize(main_window.sizeHint());
    main_window.show();

    return app.exec();
}

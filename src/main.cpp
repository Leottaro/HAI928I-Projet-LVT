#include "mainWindow.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QSurfaceFormat>

int main(int argc, char *argv[]) {
    QCoreApplication::setApplicationName("Projet 3D");
    QCoreApplication::setOrganizationName("M2 IMAGINE");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);

    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(4, 6);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);

    MainWindow main_window;
    main_window.resize(main_window.sizeHint());
    main_window.show();

    return app.exec();
}

#include "mainWindow.hpp"

#include "glWidget.hpp"

MainWindow::MainWindow() {
    m_screen = new QWidget(this);
    setCentralWidget(m_screen);

    m_glwidget = new GLWidget(this);

    setWindowTitle("Water voxel wowow");
}

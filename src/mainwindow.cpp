#include "mainWindow.hpp"

MainWindow::MainWindow() {
    m_scene = new GLScene(this);

    setCentralWidget(m_scene);
    setWindowTitle("Water voxel wowow");
}

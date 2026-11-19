#pragma once

#include "glScene.hpp"

#include <QMainWindow>

class MainWindow : public QMainWindow {
    Q_OBJECT

private:
    GLScene* m_scene;

public:
    MainWindow(QWidget* parent = nullptr);

private:
    void buildRenderMenu();
    void buildCameraMenu();
};
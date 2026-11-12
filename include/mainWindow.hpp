#pragma once

#include "glScene.hpp"

#include <QtWidgets/QMainWindow>

class MainWindow : public QMainWindow {
    Q_OBJECT

private:
    GLScene* m_scene;

public:
    MainWindow();
};
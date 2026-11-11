#pragma once

#include "glWidget.hpp"

#include <QMainWindow>

class MainWindow : public QMainWindow {
    Q_OBJECT

private:
    QWidget* m_screen;

    GLWidget* m_glwidget;

public:
    MainWindow();
};
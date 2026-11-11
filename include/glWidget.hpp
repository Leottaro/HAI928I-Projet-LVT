#pragma once

#include "Camera.hpp"

#include <qevent.h>
#include <qtypes.h>

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>

using time_point = std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<long, std::ratio<1, 1000000000>>>;

class GLWidget : public QOpenGLWidget,
                 protected QOpenGLFunctions {
    Q_OBJECT

private:
    bool m_core;
    QPoint m_last_position;
    QMetaObject::Connection m_cleanup_connection;
    std::unique_ptr<QOpenGLShaderProgram> m_program;

    quint64 m_last_frame;
    Camera m_camera;

    // TODO: RayTracingRenderer

public:
    GLWidget(QWidget *parent = 0);
    ~GLWidget();

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

private:
    void setupVertexAttrib();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void cleanupGL();
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
};
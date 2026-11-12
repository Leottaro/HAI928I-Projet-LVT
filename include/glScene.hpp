#pragma once

#include "Camera.hpp"

#include <QtCore/QElapsedTimer>
#include <QtGui/QOpenGLExtraFunctions>
#include <QtOpenGL/QOpenGLShaderProgram>
#include <QtOpenGLWidgets/QOpenGLWidget>

class GLScene : public QOpenGLWidget,
                protected QOpenGLExtraFunctions {
    Q_OBJECT

private:
    bool m_core;
    QPoint m_last_position;
    QMetaObject::Connection m_cleanup_connection;
    std::unique_ptr<QOpenGLShaderProgram> m_program;

    QElapsedTimer m_last_frame;
    QPointF m_last_mouse_pos;
    bool m_has_last_mouse_pos{false};

    Camera m_camera;

    GLuint m_grid_ssbo;
    // TODO: RayTracingRenderer

public:
    GLScene(QWidget *parent = 0);
    ~GLScene();

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

private:
    inline float getElapsedSec() const { return static_cast<float>(m_last_frame.nsecsElapsed()) * 1e-9f; }

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void cleanupGL();

    void keyPressEvent(QKeyEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *_event) override;
};
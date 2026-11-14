#pragma once

#include "Camera.hpp"
#include "Grid.hpp"

#include <QElapsedTimer>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QTimer>

class GLScene : public QOpenGLWidget,
                protected QOpenGLExtraFunctions {
    Q_OBJECT

    bool m_core{false};
    QMetaObject::Connection m_cleanup_connection;

    QPointF m_last_mouse_pos;
    bool m_has_last_mouse_pos{false};
    // Free-cam: keys currently held + a timer that moves the camera while any is down.
    QSet<int> m_pressed_keys;
    QTimer m_move_timer;
    QElapsedTimer m_move_clock;

    std::unique_ptr<QOpenGLShaderProgram> m_program;
    QOpenGLVertexArrayObject m_vao;
    GLuint m_grid_ssbo{0};

    Camera m_camera;
    FluidGrid m_grid;

public:
    explicit GLScene(QWidget *parent = nullptr);
    ~GLScene() override;

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void cleanupGL();
    void uploadGrid();
    void tickFreeCam();
};
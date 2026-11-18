#pragma once

#include "Camera.hpp"
#include "Grid.hpp"

#include <memory>

#include <QElapsedTimer>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QTimer>

class GLScene : public QOpenGLWidget,
                protected QOpenGLExtraFunctions {
    Q_OBJECT

private:
    bool m_core{false};
    QMetaObject::Connection m_cleanup_connection;

    QTimer m_update_timer;
    QElapsedTimer m_update_clock;

    QPointF m_last_mouse_pos;
    bool m_has_last_mouse_pos{false};
    // Free-cam: keys currently held + a timer that moves the camera while any is down.
    QSet<int> m_pressed_keys;

    std::unique_ptr<QOpenGLShaderProgram> m_program;
    QOpenGLVertexArrayObject m_vao;
    GLuint m_grid_ssbo{0};

    uint m_samples{1};

    Camera m_camera;
    FluidGrid m_grid;

public:
    explicit GLScene(QWidget* parent = nullptr);
    ~GLScene() override;

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;
    const Camera& camera() const { return m_camera; }

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void cleanupGL();
    void uploadGrid();

public slots:
    void updateObjects();

    void setSamples(int _samples);

    void setCamType(Camera::ControlType _type);
    void cycleCamType();
    void setCamPos(const QVector3D& _cam_pos);
    void setCamPosAxis(uint8_t axis, double v);
    void setCamFovyDegrees(float _fovy);
    void setCamNearFar(const QVector2D& _near_far);
    void setCamRotationSpeed(float _rotation_speed);
    void setCamFreeCamSpeed(float _free_cam_speed);
    void setCamDistanceToCenter(float _distance_to_center);
    void setCamZoomRate(float _zoom_rate);
    void setCamZoomRange(const QVector2D& _zoom_range);
    void setCamElasticity(float _elasticity);

signals:
    void samplesChanged(int _samples);

    void camTypeChanged(Camera::ControlType _type);
    void camPosChanged(const QVector3D& _cam_pos);
    void camFovyChanged(float _fovy);
    void camNearFarChanged(const QVector2D& _near_far);
    void camRotationSpeedChanged(float _rotation_speed);
    void camFreeSpeedChanged(float _free_cam_speed);
    void camDistanceToCenterChanged(float _distance_to_center);
    void camZoomRateChanged(float _zoom_rate);
    void camZoomRangeChanged(const QVector2D& _zoom_range);
    void camElasticityChanged(float _elasticity);
};
#pragma once

#include "Camera.hpp"
#include "Grid.hpp"

#include <qvectornd.h>

#include <QElapsedTimer>
#include <QEvent>
#include <QKeyEvent>
#include <QMetaObject>
#include <QMouseEvent>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QTimer>
#include <QVector2D>
#include <QVector3D>
#include <QWheelEvent>
#include <QWidget>

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

    uint m_samples{1u};
    QVector3D m_sun_pos{0., 1., 0.};
    QVector3D m_scattering_coefs{0.32, 0.64, 0.85};

    Camera m_camera;
    FluidGrid m_grid{
        {64, 64, 64}
    };
    bool m_grid_updated{true};

public:
    explicit GLScene(QWidget* parent = nullptr);
    ~GLScene() override;

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    const Camera& getCamera() const { return m_camera; }
    int getSamples() { return m_samples; }
    const QVector3D& getSunPos() const { return m_sun_pos; }
    const QVector3D& getScatteringCoefs() const { return m_scattering_coefs; }

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
    void setSunPos(const QVector3D& _sun_pos);
    void setSunPosAxis(uint8_t axis, double v);
    void setScatteringCoefs(const QVector3D& _scattering_coefs);
    void setScatteringCoefsAxis(uint8_t axis, double v);

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
    void sunPosChanged(const QVector3D& _sun_pos);
    void scatteringCoefsChanged(const QVector3D& _scattering_coefs);

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
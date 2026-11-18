#pragma once

#include <algorithm>
#include <array>
#include <cmath>

#include <QMatrix4x4>
#include <QVector2D>
#include <QVector3D>
#include <QWheelEvent>

inline const QVector3D VEC_ZERO{0.f, 0.f, 0.f};
inline const QVector3D VEC_RIGHT{1.f, 0.f, 0.f};
inline const QVector3D VEC_UP{0.f, 1.f, 0.f};
inline const QVector3D VEC_FRONT{0.f, 0.f, 1.f};

constexpr double PI = 3.14159265358979323846;
constexpr float PI_SAFE{PI - 1.e-3f};
constexpr float PI_2_SAFE{0.5f * PI - 1.e-3f};
constexpr float PI_4_SAFE{0.25f * PI - 1.e-3f};

// (pitch, yaw)
inline QVector3D eulerToEuclidian(const QVector2D& _angles) {
    float cosPhi = cosf(_angles.x());
    float x = cosPhi * sinf(_angles.y());
    float y = sinf(_angles.x());
    float z = cosPhi * cosf(_angles.y());
    return QVector3D(x, y, z);
}
inline QVector2D euclidianToEuler(const QVector3D& xyz) {
    float angles_x = asin(xyz.y() / xyz.length());
    float angles_y = atan2(xyz.x(), xyz.z());
    if (angles_y < 0.f)
        angles_y += 2.f * PI;
    return QVector2D(angles_x, angles_y);
}
inline float clipAnglePI(float angle) {
    while (angle < -PI)
        angle += 2.f * PI;
    while (angle > PI)
        angle -= 2.f * PI;
    return angle;
}
inline void clampOrientation(QVector2D& orientation) {
    orientation.setX(std::clamp(orientation.x(), -PI_2_SAFE, PI_2_SAFE));
    orientation.setY(clipAnglePI(orientation.y()));
}
inline void getViewVectors(const QVector2D& orientation, QVector3D& front, QVector3D& right, QVector3D& real_up) {
    front = eulerToEuclidian(orientation).normalized();
    right = QVector3D::crossProduct(front, VEC_UP).normalized();
    real_up = QVector3D::crossProduct(right, front).normalized();
}

class Camera {
public:
    enum class ControlType { Orbital, FreeCam, __COUNT };
    inline static constexpr uint NB_CONTROL_TYPES{static_cast<uint>(ControlType::__COUNT)};
    inline static constexpr std::array<const char*, NB_CONTROL_TYPES> CONTROL_TYPES_STR{"Orbital", "FreeCam"};

private:
    float m_aspect_ratio{1.f};

    QVector2D m_cam_orientation{0.f, 0.f}; // (pitch, yaw) in radians
    QVector3D m_front{0.f, 0.f, 0.f};
    QVector3D m_right{0.f, 0.f, 0.f};
    QVector3D m_real_up{0.f, 0.f, 0.f};
    QMatrix4x4 m_view{0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
    QMatrix4x4 m_projection{0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f};

public:
    ControlType m_type = ControlType::Orbital;

    QVector3D m_cam_pos{0.f, 0.f, 0.f};
    float m_fovy{static_cast<float>(PI / 2.0)}; // 90 degrees
    QVector2D m_near_far{1.e-1f, 1.e4f};
    float m_rotation_speed{0.005f}; // radians per pixel of mouse movement
    float m_free_cam_speed{1.f};    // world units per second

    const QVector3D* m_center{&VEC_ZERO};
    float m_distance_to_center{5.f};
    float m_zoom_rate{0.95f};
    QVector2D m_zoom_range{1.e-2f, 1.e8f};
    float m_elasticity{1.f};

private:
    inline void applyPosConstraint() {
        QVector3D should_pos;
        switch (m_type) {
        case ControlType::FreeCam:
            break;
        case ControlType::Orbital:
            should_pos = *m_center - m_distance_to_center * m_front;
            m_cam_pos = (1.f - m_elasticity) * m_cam_pos + m_elasticity * should_pos;
            m_front = *m_center - should_pos;
            m_cam_orientation = euclidianToEuler(m_front);
            getViewVectors(m_cam_orientation, m_front, m_right, m_real_up);
            break;
        case ControlType::__COUNT:
            break;
        }
    }

    inline void updateRenderingData() {
        m_projection.setToIdentity();
        m_projection.perspective(qRadiansToDegrees(m_fovy), m_aspect_ratio, m_near_far[0], m_near_far[1]);
        m_view.setToIdentity();
        m_view.lookAt(m_cam_pos, m_cam_pos + m_front, m_real_up);
    }

public:
    Camera() { refresh(); }

    inline const QVector3D& getCamPos() const { return m_cam_pos; }
    inline const QVector3D& getFront() const { return m_front; }
    inline const QMatrix4x4& getView() const { return m_view; }
    inline const QMatrix4x4& getProjection() const { return m_projection; }
    inline const QVector2D& getOrientation() const { return m_cam_orientation; }

    inline void refresh() {
        clampOrientation(m_cam_orientation);
        getViewVectors(m_cam_orientation, m_front, m_right, m_real_up);
        applyPosConstraint();
        updateRenderingData();
    }
    inline void setOrientation(const QVector2D& _orientation) {
        m_cam_orientation = _orientation;
        refresh();
    }
    inline void setAspectRatio(float _aspect_ratio) {
        m_aspect_ratio = _aspect_ratio;
        refresh();
    }
    inline void setType(ControlType _type) {
        m_type = _type;
        refresh();
    }
    inline void CycleType() { setType(static_cast<ControlType>((static_cast<uint8_t>(m_type) + 1) % NB_CONTROL_TYPES)); }

    inline void onKeysHeld(const QSet<int>& _keys, float _delta_time) {
        if (m_type != ControlType::FreeCam)
            return;

        auto axis = [&_keys](Qt::Key _positive, Qt::Key _negative) { return static_cast<float>(_keys.contains(_positive)) - static_cast<float>(_keys.contains(_negative)); };
        QVector3D motion(axis(Qt::Key_Space, Qt::Key_Control), axis(Qt::Key_D, Qt::Key_Q), axis(Qt::Key_Z, Qt::Key_S));
        if (motion.isNull())
            return;
        motion.normalize();

        const QVector3D flat_front = QVector3D::crossProduct(VEC_UP, m_right).normalized();
        m_cam_pos += _delta_time * m_free_cam_speed * (motion.x() * VEC_UP + motion.y() * m_right + motion.z() * flat_front);
        refresh();
    }
    inline void onWheel(QWheelEvent* _event) {
        const int y_delta = _event->angleDelta().y();
        if (y_delta == 0)
            return;

        const float zoomed = y_delta > 0 ? m_distance_to_center * m_zoom_rate : m_distance_to_center / m_zoom_rate;
        m_distance_to_center = std::clamp(zoomed, m_zoom_range.x(), m_zoom_range.y());
        refresh();
    }
    inline void onMouseMove(const QVector2D& _mouse_delta_px) {
        m_cam_orientation.setX(m_cam_orientation.x() - m_rotation_speed * _mouse_delta_px.y());
        m_cam_orientation.setY(m_cam_orientation.y() - m_rotation_speed * _mouse_delta_px.x());
        refresh();
    }
};
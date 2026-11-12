#pragma once

#include <cstdint>
#include <numbers>
#include <qevent.h>
#include <qeventpoint.h>

#include <QtGui/QKeyEvent>
#include <QtGui/QMatrix4x4>
#include <QtGui/QMouseEvent>
#include <QtGui/QMoveEvent>
#include <QtGui/qvectornd.h>

enum class ControlType { Orbital, FreeCam, __COUNT };
static constexpr uint8_t NB_CONTROL_TYPES = static_cast<uint8_t>(ControlType::__COUNT);
static constexpr const char* CONTROL_TYPES_STR = "Orbital\0FreeCam\0";

enum class RayGenMethod { Iterative, Multithreaded, GPU, __COUNT };
static constexpr uint8_t NB_RAY_GEN_METHODS = static_cast<uint8_t>(RayGenMethod::__COUNT);
static constexpr const char* RAY_GEN_METHODS_STR = "Iterative\0Multithreaded\0GPU\0";

constexpr QVector3D VEC_ZERO{0.f, 0.f, 0.f};
constexpr QVector3D VEC_RIGHT{1.f, 0.f, 0.f};
constexpr QVector3D VEC_UP{0.f, 1.f, 0.f};
constexpr QVector3D VEC_FRONT{0.f, 0.f, 1.f};

constexpr float M_PI_SAFE{std::numbers::pi - 1.e-3f};
constexpr float M_PI_2_SAFE{0.5f * std::numbers::pi - 1.e-3f};
constexpr float M_PI_4_SAFE{0.25f * std::numbers::pi - 1.e-3f};

// (pitch, yaw)
inline constexpr QVector3D eulerToEuclidian(const QVector2D& _angles) {
    float cosPhi = cosf(_angles.x());
    float x = cosPhi * sinf(_angles.y());
    float y = sinf(_angles.x());
    float z = cosPhi * cosf(_angles.y());
    return QVector3D(x, y, z);
}
inline constexpr QVector2D euclidianToEuler(const QVector3D& xyz) {
    float angles_x = asin(xyz.y() / xyz.length());
    float angles_y = atan2(xyz.x(), xyz.z());
    if (angles_y < 0.f)
        angles_y += 2.f * std::numbers::pi;
    return QVector2D(angles_x, angles_y);
}
inline constexpr float clipAnglePI(float angle) {
    while (angle < -std::numbers::pi)
        angle += 2.f * std::numbers::pi;
    while (angle > std::numbers::pi)
        angle -= 2.f * std::numbers::pi;
    return angle;
}
inline constexpr void clampOrientation(QVector2D& orientation) {
    orientation.setX(std::clamp(orientation.x(), -M_PI_2_SAFE, M_PI_2_SAFE));
    orientation.setY(clipAnglePI(orientation.y()));
}
inline constexpr void getViewVectors(const QVector2D& orientation, QVector3D& front, QVector3D& right, QVector3D& real_up) {
    front = eulerToEuclidian(orientation).normalized();
    right = QVector3D::crossProduct(front, VEC_UP).normalized();
    real_up = QVector3D::crossProduct(right, front).normalized();
}

class Camera {
    bool m_disable_mouse_actions{false};

    QVector2D m_cam_orientation{0.f, 0.f};
    QVector3D m_front{0.f, 0.f, 0.f};
    QVector3D m_right{0.f, 0.f, 0.f};
    QVector3D m_real_up{0.f, 0.f, 0.f};
    QMatrix4x4 m_view{0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
    QMatrix4x4 m_projection{0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f};

public:
    ControlType m_type = ControlType::Orbital;

    QVector3D m_cam_pos{0.f, 0.f, 0.f};
    float m_fovy{M_PI_2f};
    float m_aspect_ratio{1.f};
    QVector2D m_near_far{1.e-1f, 1.e4f};
    float m_rotation_speed{0.5f};
    float m_free_cam_speed{1.f};

    const QVector3D* m_center{&VEC_ZERO};
    float m_distance_to_center{5.f};
    float m_zoom_rate{0.95f};
    QVector2D m_zoom_range{1.e-2f, 1.e8f};
    float m_elasticity{1.f};

    RayGenMethod m_ray_gen_method{RayGenMethod::Iterative};

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
    Camera() {
        getViewVectors(m_cam_orientation, m_front, m_right, m_real_up);
        applyPosConstraint();
        updateRenderingData();
    }

    inline const QVector3D& getCamPos() const { return m_cam_pos; }
    inline const QVector3D& getFront() const { return m_front; }
    inline const QMatrix4x4& getView() const { return m_view; }
    inline const QMatrix4x4& getProjection() const { return m_projection; }

    inline void CycleType() { m_type = static_cast<ControlType>((static_cast<uint8_t>(m_type) + 1) % NB_CONTROL_TYPES); }

    // void updateInterface(ImGuiWindowFlags_ _window_flags = ImGuiWindowFlags_None) {
    //     m_disable_mouse_actions = ImGui::IsWindowHovered() || ImGui::IsAnyItemHovered() || ImGui::IsAnyItemActive() || ImGui::IsAnyItemFocused();
    //     if (!ImGui::Begin("Camera Interface", nullptr, _window_flags)) {
    //         ImGui::End();
    //         return;
    //     }

    //     // Camera Type Selection
    //     int control_type_int = static_cast<int>(m_type);
    //     if (ImGui::Combo("Camera Type", &control_type_int, CONTROL_TYPES_STR)) {
    //         m_type = static_cast<ControlType>(control_type_int);
    //     }

    //     ImGui::Separator();

    //     ImGui::BeginDisabled(m_type != ControlType::FreeCam);
    //     ImGui::DragFloat3("Position", &m_cam_pos[0], 0.1f);
    //     ImGui::EndDisabled();

    //     // Orientation Controls
    //     QVector2D angles_degree = glm::degrees(m_cam_orientation);
    //     bool pitch_changed = ImGui::DragFloat("Pitch", &angles_degree[0], .5f, -89.943f, 89.943f, "%.3f°");
    //     bool yaw_changed = ImGui::DragFloat("Yaw", &angles_degree[1], -1.f, -180.f, 180.f, "%.3f°");
    //     if (pitch_changed || yaw_changed) {
    //         m_cam_orientation = glm::radians(angles_degree);
    //         GeometryHelpers::clampOrientation<float>(m_cam_orientation);
    //         GeometryHelpers::getViewVectors(m_cam_orientation, m_front, m_right, m_real_up);
    //     }

    //     ImGui::Separator();

    //     // FOV Control
    //     float fovy_degree = glm::degrees(m_fovy);
    //     bool fovy_changed = ImGui::DragFloat("FOV", &fovy_degree, 0.1f, 1.f, 179.f, "%.3f°");
    //     if (fovy_changed) {
    //         m_fovy = glm::radians(fovy_degree);
    //     }
    //     ImGui::DragFloat2("nearFar", glm::value_ptr(m_near_far), 1.e-4f, 0.f, 1.e8f);
    //     ImGui::DragFloat("rotationSpeed", &m_rotation_speed, 1.e-2f, 0.f, 1.e2f);

    //     ImGui::Separator();

    //     ImGui::BeginDisabled(m_type != ControlType::FreeCam);
    //     ImGui::DragFloat("freeCamSpeed", &m_free_cam_speed, 1.f, 0.f, 1.e8f);
    //     ImGui::EndDisabled();

    //     ImGui::Spacing();
    //     ImGui::Separator();
    //     ImGui::Spacing();

    //     ImGui::BeginDisabled(m_type != ControlType::Orbital);
    //     ImGui::DragFloat("Distance to Center", &m_distance_to_center, 0.1f, 1.e-4f, 1.e4f);
    //     ImGui::DragFloat("Zoom Rate", &m_zoom_rate, 1.e-4f, 0.f, 1.f);
    //     ImGui::DragFloat2("Zoom Range", &m_zoom_range[0], 1.e-4f);
    //     ImGui::DragFloat("elasticity", &m_elasticity, 1.e-4f, 0.f, 1.f);
    //     ImGui::EndDisabled();

    //     ImGui::Spacing();
    //     ImGui::Separator();
    //     ImGui::Spacing();

    //     int ray_gen_int = static_cast<int>(m_ray_gen_method);
    //     if (ImGui::Combo("Ray gen method", &ray_gen_int, RAY_GEN_METHODS_STR)) {
    //         m_ray_gen_method = static_cast<RayGenMethod>(ray_gen_int);
    //     }

    //     ImGui::End();
    // }

public slots:
    inline void onAspectRatioChange(float _aspect_ratio) {
        m_aspect_ratio = _aspect_ratio;
        applyPosConstraint();
        updateRenderingData();
    }

    inline void onkeyPress(QKeyEvent* _event, float _delta_time) {
        if (m_type != ControlType::FreeCam)
            return;
        int key = _event->key();
        QVector3D motion = QVector3D(static_cast<int>(key == Qt::Key::Key_Space) - static_cast<int>(key == Qt::Key::Key_Control),
                                     static_cast<int>(key == Qt::Key::Key_D) - static_cast<int>(key == Qt::Key::Key_A),
                                     static_cast<int>(key == Qt::Key::Key_W) - static_cast<int>(key == Qt::Key::Key_S));
        if (motion.x() != 0.f || motion.y() != 0.f || motion.z() != 0.f)
            motion.normalize();
        QVector3D flat_front = QVector3D::crossProduct(VEC_UP, m_right);
        m_cam_pos += _delta_time * m_free_cam_speed * (motion.x() * VEC_UP + motion.y() * m_right + motion.z() * flat_front);
        applyPosConstraint();
        updateRenderingData();
    }

    inline void onWheel(QWheelEvent* _event) {
        if (m_disable_mouse_actions)
            return;

        int y_delta = _event->angleDelta().y();
        if (y_delta == 0)
            return;

        float new_zoom = y_delta > 0 ? m_distance_to_center * m_zoom_rate : m_distance_to_center / m_zoom_rate;
        if (new_zoom < m_zoom_range.x()) {
            new_zoom = m_zoom_range.x();
        } else if (new_zoom > m_zoom_range.y()) {
            new_zoom = m_zoom_range.y();
        }
        m_distance_to_center = new_zoom;

        clampOrientation(m_cam_orientation);
        getViewVectors(m_cam_orientation, m_front, m_right, m_real_up);
        applyPosConstraint();
        updateRenderingData();
    }

    inline void onMouseMove(const QVector2D& _mouse_vel, float _delta_time) {
        if (m_disable_mouse_actions)
            return;

        float rotation_speed = _delta_time * m_rotation_speed;
        m_cam_orientation.setX(m_cam_orientation.x() - rotation_speed * _mouse_vel.y());
        m_cam_orientation.setY(m_cam_orientation.y() - rotation_speed * _mouse_vel.x());

        clampOrientation(m_cam_orientation);
        getViewVectors(m_cam_orientation, m_front, m_right, m_real_up);

        applyPosConstraint();
        updateRenderingData();
    }
};

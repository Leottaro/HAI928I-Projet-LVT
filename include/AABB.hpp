#pragma once

#include <iostream>

#include <QtGui/QMatrix4x4>
#include <QtGui/qvectornd.h>

static constexpr std::array<QVector3D, 6> FACE_NORMALS = {
    {
     {-1.f, 0.f, 0.f}, // -X
        {0.f, -1.f, 0.f}, // -Y
        {0.f, 0.f, -1.f}, // -Z
        {1.f, 0.f, 0.f},  // +X
        {0.f, 1.f, 0.f},  // +Y
        {0.f, 0.f, 1.f},  // +Z
    }
};

struct AABB {
    static constexpr float POSITIVE_EPSILON = std::numeric_limits<float>::min();
    static constexpr float NEGATIVE_EPSILON = -std::numeric_limits<float>::min();
    static constexpr float POSITIVE_MAX = std::numeric_limits<float>::max();
    static constexpr float NEGATIVE_MAX = -std::numeric_limits<float>::max();

    QVector3D min;
    QVector3D max;

    AABB() : min(POSITIVE_MAX, POSITIVE_MAX, POSITIVE_MAX), max(NEGATIVE_MAX, NEGATIVE_MAX, NEGATIVE_MAX) {}
    AABB(const QVector3D& _min, const QVector3D& _max) : min(_min), max(_max) {}

    friend std::ostream& operator<<(std::ostream& os, const AABB& aabb) {
        os << "AABB{min: " << aabb.min.x() << ", " << aabb.min.y() << ", " << aabb.min.z() << " | max: " << aabb.max.x() << ", " << aabb.max.y() << ", " << aabb.max.z() << "}";
        return os;
    }
    friend AABB operator+(const AABB& _a, const QVector3D& _offset) { return AABB(_a.min + _offset, _a.max + _offset); }
    friend AABB operator+(const QVector3D& _offset, const AABB& _a) { return AABB(_offset + _a.min, _offset + _a.max); }

    inline void addPosition(const QVector3D& v) {
        min.setX(std::min(min.x(), v.x()));
        min.setY(std::min(min.y(), v.y()));
        min.setZ(std::min(min.z(), v.z()));
        max.setX(std::max(max.x(), v.x()));
        max.setY(std::max(max.y(), v.y()));
        max.setZ(std::max(max.z(), v.z()));
    }

    inline void expand(float value) {
        min.setX(min.x() - value);
        min.setY(min.y() - value);
        min.setZ(min.z() - value);
        max.setX(max.x() + value);
        max.setY(max.y() + value);
        max.setZ(max.z() + value);
    }
    inline void expand(const QVector3D& value) {
        min -= value;
        max += value;
    }

    inline QVector3D getCorner(int idx) const { return QVector3D((idx & 1) ? max.x() : min.x(), (idx & 2) ? max.y() : min.y(), (idx & 4) ? max.z() : min.z()); }

    template<typename Func>
    inline void forAllCorners(Func&& _func) const {
        for (int i = 0; i < 8; ++i) {
            _func(getCorner(i));
        }
    }

    inline bool isInside(const QVector3D& v) const { return !(v.x() < min.x() || v.y() < min.y() || v.z() < min.z() || v.x() > max.x() || v.y() > max.y() || v.z() > max.z()); }

    // Face indices: -X=0, -Y=1, -Z=2, +X=3, +Y=4, +Z=5
    inline bool intersectRay(const QVector3D& origin, const QVector3D& direction, float& tmin, float& tmax, uint& face_min, uint& face_max) const {
        QVector3D delta_min = min - origin;
        QVector3D delta_max = max - origin;

        float t0, t1;
        int f0, f1;

        // X slab: -X=0, +X=3
        t0 = delta_min.x() / direction.x();
        t1 = delta_max.x() / direction.x();
        f0 = 0;
        f1 = 3; // -X, +X
        if (t0 > t1) {
            std::swap(t0, t1);
            std::swap(f0, f1);
        }
        tmin = t0;
        tmax = t1;
        face_min = f0;
        face_max = f1;

        // Y slab: -Y=1, +Y=4
        float tmin_tmp = delta_min.y() / direction.y();
        float tmax_tmp = delta_max.y() / direction.y();
        int fmin_tmp = 1, fmax_tmp = 4; // -Y, +Y
        if (tmin_tmp > tmax_tmp) {
            std::swap(tmin_tmp, tmax_tmp);
            std::swap(fmin_tmp, fmax_tmp);
        }

        if (tmax_tmp < tmin || tmin_tmp > tmax)
            return false;
        if (tmin_tmp > tmin) {
            tmin = tmin_tmp;
            face_min = fmin_tmp;
        }
        if (tmax_tmp < tmax) {
            tmax = tmax_tmp;
            face_max = fmax_tmp;
        }

        // Z slab: -Z=2, +Z=5
        tmin_tmp = delta_min.z() / direction.z();
        tmax_tmp = delta_max.z() / direction.z();
        fmin_tmp = 2;
        fmax_tmp = 5; // -Z, +Z
        if (tmin_tmp > tmax_tmp) {
            std::swap(tmin_tmp, tmax_tmp);
            std::swap(fmin_tmp, fmax_tmp);
        }

        if (tmax_tmp < tmin || tmin_tmp > tmax)
            return false;
        if (tmin_tmp > tmin) {
            tmin = tmin_tmp;
            face_min = fmin_tmp;
        }
        if (tmax_tmp < tmax) {
            tmax = tmax_tmp;
            face_max = fmax_tmp;
        }

        return true;
    }
};

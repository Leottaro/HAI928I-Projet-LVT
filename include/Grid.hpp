#pragma once

#include <cstddef>
#include <functional>
#include <glm/glm.hpp>
#include <qvector3d.h>
#include <vector>

#include <QVector3D>

template<typename CellData>
class GridBase {
protected:
    glm::uvec3 m_size;
    size_t m_cell_number;
    QVector3D m_cell_dims;
    QVector3D m_pos;
    std::vector<CellData> m_data;

public:
    GridBase(const glm::uvec3& _size, const CellData& _base = CellData{}) :
        m_size(_size), m_cell_number(_size.x * _size.y * _size.z), m_cell_dims(1.f / _size.x, 1.f / _size.y, 1.f / _size.z), m_pos(-0.5f * (QVector3D(m_size.x, m_size.y, m_size.z) * m_cell_dims)),
        m_data(m_cell_number, _base) {}
    GridBase(const CellData& _base = CellData{}) : GridBase<CellData>(glm::uvec3{32, 32, 32}, _base) {}

    inline const glm::uvec3& getSize() const { return m_size; }
    inline size_t getCellNumber() const { return m_cell_number; }
    inline const QVector3D& getCellDims() const { return m_cell_dims; }
    inline const QVector3D& getPos() const { return m_pos; }

    inline const CellData& getCell(size_t i) const { return m_data[i]; }
    inline const CellData& getCell(const glm::uvec3& _cell_pos) const { return m_data[cellPosToIndexZorder(_cell_pos)]; }
    inline CellData& getCell(size_t i) { return m_data[i]; }
    inline CellData& getCell(const glm::uvec3& _cell_pos) { return m_data[cellPosToIndexZorder(_cell_pos)]; }
    inline const std::vector<CellData>& data() const { return m_data; }
    inline std::vector<CellData>& data() { return m_data; }

    inline CellData getCellOrZero(const glm::ivec3 & p_cell_pos) const{
        uint index = cellPosToIndexZorder(p_cell_pos);
        return (index > m_size.x * m_size.y * m_size.z) ? 0. : m_data[index];
    }

    inline glm::uvec3 posToCellPos(const QVector3D& _pos) const {
        QVector3D rel = ((_pos - m_pos) / m_cell_dims);
        glm::uvec3 res{0, 0, 0};
        for (uint i = 0; i < 3; i++)
            res[i] = std::max(res[i], uint(rel[i]));
        return res;
    }

    inline uint cellPosToIndexZorder(glm::uvec3 _cell_pos) const {
        for (uint dim = 0; dim < 3; dim++) {
            _cell_pos[dim] = (_cell_pos[dim] | (_cell_pos[dim] << 16)) & uint(0xFF0000FF);
            _cell_pos[dim] = (_cell_pos[dim] | (_cell_pos[dim] << 8)) & uint(0x0F00F00F);
            _cell_pos[dim] = (_cell_pos[dim] | (_cell_pos[dim] << 4)) & uint(0xC30C30C3);
            _cell_pos[dim] = (_cell_pos[dim] | (_cell_pos[dim] << 2)) & uint(0x49249249);
        }

        uint res = 0;
        for (uint dim = 0; dim < 3; dim++)
            res |= (_cell_pos[dim] << dim);
        return res;
    }
    inline uint posToIndexZorder(const QVector3D& _pos) const { return cellPosToIndexZorder(posToCellPos(_pos)); }

    inline void forEachCell(const std::function<void(size_t, CellData&)>& _f) {
        for (size_t i = 0; i < m_cell_number; i++)
            _f(i, getCell(i));
    }
    inline void forEachCell(const std::function<void(size_t, const glm::uvec3&, CellData&)>& _f) {
        glm::uvec3 cell_pos;
        for (cell_pos.z = 0; cell_pos.z < m_size.z; cell_pos.z++) {
            for (cell_pos.y = 0; cell_pos.y < m_size.y; cell_pos.y++) {
                for (cell_pos.x = 0; cell_pos.x < m_size.x; cell_pos.x++) {
                    size_t i = cellPosToIndexZorder(cell_pos);
                    _f(i, cell_pos, getCell(i));
                }
            }
        }
    }
    inline void forEachCell(const std::function<void(size_t, const CellData&)>& _f) const {
        for (size_t i = 0; i < m_cell_number; i++)
            _f(i, getCell(i));
    }
    inline void forEachCell(const std::function<void(size_t, const glm::uvec3&, const CellData&)>& _f) const {
        glm::uvec3 cell_pos;
        for (cell_pos.z = 0; cell_pos.z < m_size.z; cell_pos.z++) {
            for (cell_pos.y = 0; cell_pos.y < m_size.y; cell_pos.y++) {
                for (cell_pos.x = 0; cell_pos.x < m_size.x; cell_pos.x++) {
                    size_t i = cellPosToIndexZorder(cell_pos);
                    _f(i, cell_pos, getCell(i));
                }
            }
        }
    }

    CellData sample(const glm::vec3 & p_position);
};

// Linear interpolation from p_from to p_to
template <typename T>
T lerp(const T & p_from, const T & p_to, const float p_weight) {
    return p_from + p_weight * (p_to - p_from);
}


// Interpolate the grid at a real position, in between vertices
template<> float GridBase<float>::sample(const glm::vec3 & p_grid_position) {
    glm::ivec3 grid_cell_position = glm::floor(p_grid_position);  // TODO : réfléchir à peut être faire plaisir à léo et mettre un -m_pos
    glm::vec3 pos_in_cell = p_grid_position - glm::vec3(grid_cell_position);

    float lx_ly_lz = getCellOrZero(grid_cell_position);
    float hx_ly_lz = getCellOrZero(grid_cell_position + glm::ivec3(1, 0, 0));
    float lx_hy_lz = getCellOrZero(grid_cell_position + glm::ivec3(0, 1, 0));
    float hx_hy_lz = getCellOrZero(grid_cell_position + glm::ivec3(1, 1, 0));
    float lx_ly_hz = getCellOrZero(grid_cell_position + glm::ivec3(0, 0, 1));
    float hx_ly_hz = getCellOrZero(grid_cell_position + glm::ivec3(1, 0, 1));
    float lx_hy_hz = getCellOrZero(grid_cell_position + glm::ivec3(0, 1, 1));
    float hx_hy_hz = getCellOrZero(grid_cell_position + glm::ivec3(1, 1, 1));

    float lx_ly = lerp(lx_ly_lz, lx_ly_hz, pos_in_cell.z);
    float hx_ly = lerp(hx_ly_lz, hx_ly_hz, pos_in_cell.z);
    float lx_hy = lerp(lx_hy_lz, lx_hy_hz, pos_in_cell.z);
    float hx_hy = lerp(hx_hy_lz, hx_hy_hz, pos_in_cell.z);

    float lx = lerp(lx_ly, lx_hy, pos_in_cell.y);
    float hx = lerp(hx_ly, hx_hy, pos_in_cell.y);

    return lerp(lx, hx, pos_in_cell.x);
}




class FluidGrid {
    glm::uvec3 m_size;

    float voxel_size;
    float density;

    GridBase<float> m_density;

    GridBase<float> m_velocity_x;
    GridBase<float> m_velocity_y;
    GridBase<float> m_velocity_z;

public:
    //       ___             _               _
    //      / __|___ _ _  __| |_ _ _ _  _ __| |_ ___ _ _ ___
    //     | (__/ _ \ ' \(_-<  _| '_| || / _|  _/ _ \ '_(_-<
    //      \___\___/_||_/__/\__|_|  \_,_\__|\__\___/_| /__/
    //

    FluidGrid(const glm::uvec3& p_size):
        m_size(p_size),
        m_velocity_x(p_size + glm::uvec3(1, 0, 0)),
        m_velocity_y(p_size + glm::uvec3(0, 1, 0)),
        m_velocity_z(p_size + glm::uvec3(0, 0, 1))
        {}

    FluidGrid(){}

    //       ___     _   _                __       ___      _   _
    //      / __|___| |_| |_ ___ _ _ ___ / _|___  / __| ___| |_| |_ ___ _ _ ___
    //     | (_ / -_)  _|  _/ -_) '_(_-< > _|_ _| \__ \/ -_)  _|  _/ -_) '_(_-<
    //      \___\___|\__|\__\___|_| /__/ \_____|  |___/\___|\__|\__\___|_| /__/
    //

    inline const GridBase<QVector3D>& getVelocity() const {
        return m_velocities_grid;
    }

    //      ___ _           _
    //     | _ \ |_ _  _ __(_)__ ___
    //     |  _/ ' \ || (_-< / _(_-<
    //     |_| |_||_\_, /__/_\__/__/
    //              |__/


    void integrateVelocity(float p_delta_time) {

    }


    // Integrate physics by one time step
    void update(float p_delta_time) {
        // forEachCell([this](size_t i, const glm::uvec3& pos, float& density) { density *= rand() % 2 ? 1.f / 0.95f : 0.95f; });
    }
};
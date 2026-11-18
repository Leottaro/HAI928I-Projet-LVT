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
};

class FluidGrid : public GridBase<float> {
    GridBase<QVector3D> m_velocities_grid;

public:
    FluidGrid(const glm::uvec3& _size) : GridBase<float>(_size), m_velocities_grid(_size) {}
    FluidGrid() : GridBase<float>(), m_velocities_grid() {}

    inline const GridBase<QVector3D>& velocities() const { return m_velocities_grid; }

    inline void update(float _delta_time) { // TODO: c'est fluide ou quoi là
        forEachCell([this](size_t i, const glm::uvec3& pos, float& density) { density *= rand() % 2 ? 1.f / 0.95f : 0.95f; });
    }
};
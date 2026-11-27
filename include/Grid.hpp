#pragma once

#include "glm/ext/vector_int3.hpp"
#include <cstddef>
#include <functional>
#include <glm/glm.hpp>
#include <qvector3d.h>
#include <vector>

#include <QVector3D>




// Linear interpolation from p_from to p_to
template <typename T>
T lerp(const T & p_from, const T & p_to, const float p_weight) {
    return p_from + p_weight * (p_to - p_from);
}

template<typename CellData>
class GridBase {
protected:
    glm::uvec3 m_size;  // The dimensions of the grid
    std::vector<CellData> m_data;

public:
    //       ___             _               _
    //      / __|___ _ _  __| |_ _ _ _  _ __| |_ ___ _ _ ___
    //     | (__/ _ \ ' \(_-<  _| '_| || / _|  _/ _ \ '_(_-<
    //      \___\___/_||_/__/\__|_|  \_,_\__|\__\___/_| /__/
    //

    GridBase(const glm::uvec3& p_size, const CellData& p_default = CellData{}):
        m_size(p_size),
        m_data(p_size.x * p_size.y * p_size.z, p_default)
    {}

    //      ____   ___         _
    //     |_  /  / _ \ _ _ __| |___ _ _
    //      / /  | (_) | '_/ _` / -_) '_|
    //     /___|  \___/|_| \__,_\___|_|
    //

    // Convert a cell position as a uvec3 to its index in the stored m_data vector
    inline uint cellPosToIndexZorder(glm::uvec3 p_cell_pos) const {
        for (uint dim = 0; dim < 3; dim++) {
            p_cell_pos[dim] = (p_cell_pos[dim] | (p_cell_pos[dim] << 1)) & uint(0x00FF00FF);
            p_cell_pos[dim] = (p_cell_pos[dim] | (p_cell_pos[dim] << 2)) & uint(0x0F0F0F0F);
            p_cell_pos[dim] = (p_cell_pos[dim] | (p_cell_pos[dim] << 4)) & uint(0x33333333);
            p_cell_pos[dim] = (p_cell_pos[dim] | (p_cell_pos[dim] << 8)) & uint(0x55555555);
        }

        uint res = 0;
        for (uint dim = 0; dim < 3; dim++)
            res |= (p_cell_pos[dim] << dim);
        return res;
    }

    //       ___     _   _                __       ___      _   _
    //      / __|___| |_| |_ ___ _ _ ___ / _|___  / __| ___| |_| |_ ___ _ _ ___
    //     | (_ / -_)  _|  _/ -_) '_(_-< > _|_ _| \__ \/ -_)  _|  _/ -_) '_(_-<
    //      \___\___|\__|\__\___|_| /__/ \_____|  |___/\___|\__|\__\___|_| /__/
    //

    inline const std::vector<CellData> & getDataVector() const {
        return m_data;
    }

    inline std::vector<CellData> & getDataVector() {
        return m_data;
    }

    inline const glm::uvec3& getSize() const {
        return m_size;
    }

    inline size_t getCellCount() const {
        return m_size.x * m_size.y * m_size.z;
    }

    inline CellData getCell(const glm::uvec3& p_cell_pos) const {
        return m_data[cellPosToIndexZorder(p_cell_pos)];
    }

    inline CellData getCell(size_t p_index) const{
        return m_data[p_index];
    }

    inline CellData getCellOrElse(const glm::ivec3 & p_cell_pos, const CellData& p_else) const {
        return
            (p_cell_pos.x < 0 or p_cell_pos.x >= m_size.x or p_cell_pos.y < 0 or p_cell_pos.y >= m_size.y or p_cell_pos.z < 0 or p_cell_pos.z >= m_size.z)
        ?
            p_else
        :
            getCell(p_cell_pos)
        ;
    }

    inline const CellData & getCellRef(const glm::uvec3 & p_cell_pos) const{
        return m_data[cellPosToIndexZorder(p_cell_pos)];
    }

    inline CellData & getCellRef(const glm::uvec3 & p_cell_pos){
        return m_data[cellPosToIndexZorder(p_cell_pos)];
    }

    // inline uint posToIndexZorder(const QVector3D& _pos) const { return cellPosToIndexZorder(posToCellPos(_pos)); }


    //      ___             _   _               _
    //     | __|  _ _ _  __| |_(_)___ _ _  __ _| |
    //     | _| || | ' \/ _|  _| / _ \ ' \/ _` | |
    //     |_| \_,_|_||_\__|\__|_\___/_||_\__,_|_|
    //
    inline void forEachCell(const std::function<void(CellData&)>& p_function) {
        for (size_t i = 0; i < getCellCount(); i++)
            p_function(m_data[i]);
    }

    inline void forEachCell(const std::function<void(size_t, CellData&)>& p_function) {
        for (size_t i = 0; i < getCellCount(); i++)
            p_function(i, m_data[i]);
    }

    inline void forEachCell(const std::function<void(const glm::uvec3&, CellData&)>& p_function) {
        glm::uvec3 cell_pos;
        for (cell_pos.z = 0; cell_pos.z < m_size.z; cell_pos.z++) {
            for (cell_pos.y = 0; cell_pos.y < m_size.y; cell_pos.y++) {
                for (cell_pos.x = 0; cell_pos.x < m_size.x; cell_pos.x++) {
                    size_t i = cellPosToIndexZorder(cell_pos);
                    p_function(cell_pos, m_data[i]);
                }
            }
        }
    }

    // This function doesn't explore the grid in the order it is stored in, and so causes cache misses.
    // When knowing the position in the grid as a (x, y, z) isn't needed, use the other signature instead
    inline void forEachCell(const std::function<void(size_t, const glm::uvec3&, CellData&)>& p_function) {
        glm::uvec3 cell_pos;
        for (cell_pos.z = 0; cell_pos.z < m_size.z; cell_pos.z++) {
            for (cell_pos.y = 0; cell_pos.y < m_size.y; cell_pos.y++) {
                for (cell_pos.x = 0; cell_pos.x < m_size.x; cell_pos.x++) {
                    size_t i = cellPosToIndexZorder(cell_pos);
                    p_function(i, cell_pos, m_data[i]);
                }
            }
        }
    }

    inline void forEachCell(const std::function<void(size_t, const CellData&)>& _f) const {
        for (size_t i = 0; i < getCellCount(); i++)
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


/*
    Sample a scalar field at a position in cell units, potentially in between grid vertices.
    The offset corresponds to how the grid is centered.
    Use an offset of (0.5, 0.5, 0.5) for cell centered grids (density, ...) and an offset of (0.5, 0., 0.) for face centered grids (velocity)
    Vertex values outside of the field are treated as the provided default value
    
    NOTE : Yes, p_offset isn't that useful and could just be subtracted to p_position when calling this function,
    but it's useful to unload that logic here to avoid making sign mistakes down the road
*/
float sample(const GridBase<float> & p_field, const glm::vec3 & p_position, const glm::vec3 & p_offset, const float p_default = 0.f);

/*CellData sample(const glm::vec3 & p_position) const{

        glm::ivec3 grid_cell_position = glm::floor(p_position);  // TODO : réfléchir à peut être faire plaisir à léo et mettre un -m_pos
        glm::vec3 pos_in_cell = p_position - glm::vec3(grid_cell_position);

        CellData lx_ly_lz = getCellOrElse(grid_cell_position, 0.f);
        CellData hx_ly_lz = getCellOrElse(grid_cell_position + glm::ivec3(1, 0, 0), 0.f);
        CellData lx_hy_lz = getCellOrElse(grid_cell_position + glm::ivec3(0, 1, 0), 0.f);
        CellData hx_hy_lz = getCellOrElse(grid_cell_position + glm::ivec3(1, 1, 0), 0.f);
        CellData lx_ly_hz = getCellOrElse(grid_cell_position + glm::ivec3(0, 0, 1), 0.f);
        CellData hx_ly_hz = getCellOrElse(grid_cell_position + glm::ivec3(1, 0, 1), 0.f);
        CellData lx_hy_hz = getCellOrElse(grid_cell_position + glm::ivec3(0, 1, 1), 0.f);
        CellData hx_hy_hz = getCellOrElse(grid_cell_position + glm::ivec3(1, 1, 1), 0.f);

        CellData lx_ly = lerp(lx_ly_lz, lx_ly_hz, pos_in_cell.z);
        CellData hx_ly = lerp(hx_ly_lz, hx_ly_hz, pos_in_cell.z);
        CellData lx_hy = lerp(lx_hy_lz, lx_hy_hz, pos_in_cell.z);
        CellData hx_hy = lerp(hx_hy_lz, hx_hy_hz, pos_in_cell.z);


        CellData lx = lerp(lx_ly, lx_hy, pos_in_cell.y);
        CellData hx = lerp(hx_ly, hx_hy, pos_in_cell.y);

        return lerp(lx, hx, pos_in_cell.x);
    }*/


class EulerianFluidGrid {
    glm::uvec3 m_size;

    // Information regarding the grid in the world
    glm::vec3 m_world_pos;
    glm::vec3 m_world_cell_size;

    // Physics property
    float m_voxel_size;
    // float m_density;

    // Data
    GridBase<float> m_density;
    GridBase<float> m_pressure;

    GridBase<float> m_velocity_x;
    GridBase<float> m_velocity_y;
    GridBase<float> m_velocity_z;

    GridBase<bool> m_fluid;  // A grid that stores if the cell is fluid or not (ie not solid walls)

public:
    //       ___             _               _
    //      / __|___ _ _  __| |_ _ _ _  _ __| |_ ___ _ _ ___
    //     | (__/ _ \ ' \(_-<  _| '_| || / _|  _/ _ \ '_(_-<
    //      \___\___/_||_/__/\__|_|  \_,_\__|\__\___/_| /__/
    //

    EulerianFluidGrid(const glm::uvec3& p_size):
        m_size(p_size),
        m_density(p_size, 0.f),
        m_pressure(p_size, 0.f),
        m_velocity_x(p_size - glm::uvec3(1, 0, 0), 0.f),
        m_velocity_y(p_size - glm::uvec3(0, 1, 0), 0.f),
        m_velocity_z(p_size - glm::uvec3(0, 0, 1), 0.f),
        m_fluid(p_size, true)
        {}

    // EulerianFluidGrid(){}

    //       ___     _   _                __       ___      _   _
    //      / __|___| |_| |_ ___ _ _ ___ / _|___  / __| ___| |_| |_ ___ _ _ ___
    //     | (_ / -_)  _|  _/ -_) '_(_-< > _|_ _| \__ \/ -_)  _|  _/ -_) '_(_-<
    //      \___\___|\__|\__\___|_| /__/ \_____|  |___/\___|\__|\__\___|_| /__/
    //

    inline const GridBase<float> & getDensity() const {
        return m_density;
    }

    inline GridBase<float> & getDensity() {
        return m_density;
    }

    inline glm::ivec3 getSize() const {
        return m_size;
    }

    inline glm::vec3 getWorldPos() const {
        return m_world_pos;
    }

    inline void setWorldPos(const glm::vec3 & p_world_pos) {
        m_world_pos = p_world_pos;
    }

    inline glm::vec3 getWorldCellSize() const {
        return m_world_cell_size;
    }

    inline void setWorldCellSize(const glm::vec3 & p_world_cell_size) {
        m_world_cell_size = p_world_cell_size;
    }

    inline bool isCellInBounds(const glm::ivec3 & p_cell) const {
        return p_cell.x >= 0 and p_cell.y >= 0 and p_cell.z >= 0 and uint(p_cell.x) < m_size.x and uint(p_cell.y) < m_size.y and uint(p_cell.z) < m_size.z;
    }

    inline bool isCellFluid(const glm::ivec3 & p_cell) const {
        return m_fluid.getCellOrElse(p_cell, false);
    }

    float getCellDivergence(const glm::uvec3 & p_cell_index) const;

    float getCellAverageSurroundingPressure(const glm::uvec3 & p_cell) const;

    // (0.f, 0.f, 0.f) corresponds to the center of the (0, 0, 0) cell
    glm::vec3 sampleVelocity(const glm::vec3 & p_position) const;



    //      ___ _           _
    //     | _ \ |_ _  _ __(_)__ ___
    //     |  _/ ' \ || (_-< / _(_-<
    //     |_| |_||_\_, /__/_\__/__/
    //              |__/


    // Integrate velocity using acceleration
    void integrateVelocity(float p_delta_time);


    // Move around all quantities of the fluid
    GridBase<float> advect(const GridBase<float> & p_field, const float p_delta_time, const glm::vec3 & p_offset) const;
    void advection(float _p_delta_time);


    // Integrate physics by one time step
    void update(float p_delta_time);

    void pressureSolverStep(const float p_delta_time);
    void solvePressure(const float p_delta_time);
    void projection(const float p_delta_time);

    void endStep();

    //       ___  _   _
    //      / _ \| |_| |_  ___ _ _
    //     | (_) |  _| ' \/ -_) '_|
    //      \___/ \__|_||_\___|_|
    //
    void randomizeDensity() {
        m_density.forEachCell(
            [](float & density) {
                density = static_cast<float>(rand()) / RAND_MAX;
            }
        );
    }
};
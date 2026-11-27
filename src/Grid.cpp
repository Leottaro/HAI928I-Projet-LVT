#include "Grid.hpp"
#include <iostream>

const float GRAVITY = -9.81;  // Gravity, in m.s-1




float sample(const GridBase<float> & p_field, const glm::vec3 & p_position, const float p_default) {
    glm::ivec3 cell_position = glm::floor(p_position);
    glm::vec3 t = p_position - glm::vec3(cell_position);

    float lx_ly_lz = p_field.getCellOrElse(cell_position, p_default);
    float hx_ly_lz = p_field.getCellOrElse(cell_position + glm::ivec3(1, 0, 0), p_default);
    float lx_hy_lz = p_field.getCellOrElse(cell_position + glm::ivec3(0, 1, 0), p_default);
    float hx_hy_lz = p_field.getCellOrElse(cell_position + glm::ivec3(1, 1, 0), p_default);
    float lx_ly_hz = p_field.getCellOrElse(cell_position + glm::ivec3(0, 0, 1), p_default);
    float hx_ly_hz = p_field.getCellOrElse(cell_position + glm::ivec3(1, 0, 1), p_default);
    float lx_hy_hz = p_field.getCellOrElse(cell_position + glm::ivec3(0, 1, 1), p_default);
    float hx_hy_hz = p_field.getCellOrElse(cell_position + glm::ivec3(1, 1, 1), p_default);

    float lx_ly = lerp(lx_ly_lz, lx_ly_hz, t.z);
    float hx_ly = lerp(hx_ly_lz, hx_ly_hz, t.z);
    float lx_hy = lerp(lx_hy_lz, lx_hy_hz, t.z);
    float hx_hy = lerp(hx_hy_lz, hx_hy_hz, t.z);


    float lx = lerp(lx_ly, lx_hy, t.y);
    float hx = lerp(hx_ly, hx_hy, t.y);

    return lerp(lx, hx, t.x);
}


glm::vec3 EulerianFluidGrid::sampleVelocity(const glm::vec3 & p_position) const {
    return glm::vec3(
        sample(m_velocity_x, p_position - glm::vec3(0.5f, 0.0f, 0.0f), 0.f),
        sample(m_velocity_y, p_position - glm::vec3(0.0f, 0.5f, 0.0f), 0.f),
        sample(m_velocity_z, p_position - glm::vec3(0.0f, 0.0f, 0.5f), 0.f)
    );
}


GridBase<float> EulerianFluidGrid::advect(const GridBase<float> & p_field, const float p_delta_time, const glm::vec3 & p_offset) const {
    std::cout << "\t\tcreating new field" << std::endl;
    GridBase<float> new_field = GridBase<float>(p_field.getSize(), 0.f);

    std::cout << "\t\tcomputing new field" << std::endl;
    new_field.forEachCell(
        [&](const glm::uvec3 & cell, float & quantity) {
            glm::vec3 velocity = sampleVelocity(glm::vec3(cell) + p_offset);
            quantity = sample(p_field, glm::vec3(cell) - p_delta_time * velocity, 0.f);
        }
    );

    return new_field;

    // p_field.forEachCell(
    //     [&](const glm::uvec3 & cell, float & quantity) {
    //         quantity = new_field.getCell(cell);
    //     }
    // );
}


void replace(GridBase<float> & p_old_field, const GridBase<float> & p_new_field) {
    // p_old_field.getDataVector() = p_new_field.getDataVector();
    // p_old_field = p_new_field;

    p_old_field.forEachCell(
        [&](size_t index, float & quantity){
            quantity = p_new_field.getDataVector()[index];
        }
    );
}


void EulerianFluidGrid::advection(float p_delta_time) {
    std::cout << "\tnew velocity" << std::endl;
    std::cout << "x size : " << m_velocity_x.getSize().x << "," << m_velocity_x.getSize().y << "," << m_velocity_x.getSize().z << std::endl;
    std::cout << "y size : " << m_velocity_y.getSize().x << "," << m_velocity_y.getSize().y << "," << m_velocity_y.getSize().z << std::endl;
    std::cout << "z size : " << m_velocity_z.getSize().x << "," << m_velocity_z.getSize().y << "," << m_velocity_z.getSize().z << std::endl;
    GridBase<float> new_velocity_x = advect(m_velocity_x, p_delta_time, glm::vec3(0.5f, 0.0f, 0.0f));
    GridBase<float> new_velocity_y = advect(m_velocity_y, p_delta_time, glm::vec3(0.0f, 0.5f, 0.0f));
    GridBase<float> new_velocity_z = advect(m_velocity_z, p_delta_time, glm::vec3(0.0f, 0.0f, 0.5f));

    std::cout << "\tdensity" << std::endl;
    replace(
        m_density,
        advect(m_density, p_delta_time, glm::vec3(0.f, 0.f, 0.f))
    );

    std::cout << "\tstoring new velocity" << std::endl;
    replace(m_velocity_x, new_velocity_x);
    replace(m_velocity_y, new_velocity_y);
    replace(m_velocity_z, new_velocity_z);
}


void EulerianFluidGrid::integrateVelocity(float p_delta_time) {
    m_velocity_y.forEachCell(
        [p_delta_time](float & p_velocity){
            float acceleration = 0.f;  // in m.s-2
            // acceleration += GRAVITY;
            p_velocity += acceleration * p_delta_time;
        }
    );
}

float EulerianFluidGrid::getCellDivergence(const glm::uvec3 & p_cell_index) const {
    float divergence = 0.f;

    divergence += isCellFluid(p_cell_index + glm::uvec3(+1, 0, 0)) ? m_velocity_x.getCell(p_cell_index                      ) : 0.f;
    divergence -= isCellFluid(p_cell_index + glm::uvec3(-1, 0, 0)) ? m_velocity_x.getCell(p_cell_index - glm::uvec3(1, 0, 0)) : 0.f;

    divergence += isCellFluid(p_cell_index + glm::uvec3(0, +1, 0)) ? m_velocity_y.getCell(p_cell_index                      ) : 0.f;
    divergence -= isCellFluid(p_cell_index + glm::uvec3(0, -1, 0)) ? m_velocity_y.getCell(p_cell_index - glm::uvec3(0, 1, 0)) : 0.f;

    divergence += isCellFluid(p_cell_index + glm::uvec3(0, 0, +1)) ? m_velocity_z.getCell(p_cell_index                      ) : 0.f;
    divergence -= isCellFluid(p_cell_index + glm::uvec3(0, 0, -1)) ? m_velocity_z.getCell(p_cell_index - glm::uvec3(0, 0, 1)) : 0.f;


    return divergence;
}

float EulerianFluidGrid::getCellAverageSurroundingPressure(const glm::uvec3 & p_cell) const {
    float average_pressure = 0.f;
    int neighbor_count = 0;

    glm::ivec3 i_cell = glm::ivec3(p_cell);
    
    glm::ivec3 pos_x = i_cell + glm::ivec3(+1, 0, 0);
    bool pos_x_fluid = isCellFluid(pos_x);
    average_pressure += pos_x_fluid ? m_pressure.getCell(pos_x) : 0.;
    neighbor_count += pos_x_fluid ? 1 : 0;

    glm::ivec3 neg_x = i_cell + glm::ivec3(-1, 0, 0);
    bool neg_x_fluid = isCellFluid(neg_x);
    average_pressure += neg_x_fluid ? m_pressure.getCell(neg_x) : 0.;
    neighbor_count += neg_x_fluid ? 1 : 0;


    glm::ivec3 pos_y = i_cell + glm::ivec3(0, +1, 0);
    bool pos_y_fluid = isCellFluid(pos_y);
    average_pressure += pos_y_fluid ? m_pressure.getCell(pos_y) : 0.;
    neighbor_count += pos_y_fluid ? 1 : 0;

    glm::ivec3 neg_y = i_cell + glm::ivec3(0, -1, 0);
    bool neg_y_fluid = isCellFluid(neg_y);
    average_pressure += neg_y_fluid ? m_pressure.getCell(neg_y) : 0.;
    neighbor_count += neg_y_fluid ? 1 : 0;


    glm::ivec3 pos_z = i_cell + glm::ivec3(0, 0, +1);
    bool pos_z_fluid = isCellFluid(pos_z);
    average_pressure += pos_z_fluid ? m_pressure.getCell(pos_z) : 0.;
    neighbor_count += pos_z_fluid ? 1 : 0;

    glm::ivec3 neg_z = i_cell + glm::ivec3(0, 0, -1);
    bool neg_z_fluid = isCellFluid(neg_z);
    average_pressure += neg_z_fluid ? m_pressure.getCell(neg_z) : 0.;
    neighbor_count += neg_z_fluid ? 1 : 0;

    return (neighbor_count == 0) ? 0. : average_pressure / float(neighbor_count);
}

void EulerianFluidGrid::pressureSolverStep(const float p_delta_time) {
    m_pressure.forEachCell(
        [&](const glm::uvec3 & cell_pos, float & pressure) {
            pressure = getCellAverageSurroundingPressure(cell_pos) + m_world_cell_size.x * getCellDivergence(cell_pos) / p_delta_time;
        }
    );
}


void EulerianFluidGrid::solvePressure(const float p_delta_time) {
    for (uint i = 0 ; i < 10 ; i++) {
        pressureSolverStep(p_delta_time);
    }
}


void EulerianFluidGrid::projection(const float p_delta_time) {
    solvePressure(p_delta_time);

    m_velocity_x.forEachCell(
        [&](const glm::uvec3 & p_cell, float & velocity){
            float pressure_delta = m_pressure.getCell(p_cell + glm::uvec3(1, 0, 0)) - m_pressure.getCell(p_cell);
            velocity -= (p_delta_time / m_world_cell_size.x) * pressure_delta;
        }
    );

    m_velocity_y.forEachCell(
        [&](const glm::uvec3 & p_cell, float & velocity){
            float pressure_delta = m_pressure.getCell(p_cell + glm::uvec3(0, 1, 0)) - m_pressure.getCell(p_cell);
            velocity -= (p_delta_time / m_world_cell_size.y) * pressure_delta;
        }
    );

    m_velocity_z.forEachCell(
        [&](const glm::uvec3 & p_cell, float & velocity){
            float pressure_delta = m_pressure.getCell(p_cell + glm::uvec3(0, 0, 1)) - m_pressure.getCell(p_cell);
            velocity -= (p_delta_time / m_world_cell_size.z) * pressure_delta;
        }
    );
}

void EulerianFluidGrid::endStep() {

}

/*
    Integrate physics by one time step.
    A step is decomposed in the following steps :
    - Advection : Move the fluid (and each of its quantities) around
    - Forces : Integrate external forces like gravity, wind, ...
    - Pressure : Solve the divergence 
    - Some housekeeping like boundary conditions, water volume correction, ect
*/
void EulerianFluidGrid::update(float p_delta_time) {
    std::cout << "update" << std::endl;

    std::cout << "advection" << std::endl;
    advection(p_delta_time);
    
    std::cout << "integration" << std::endl;
    integrateVelocity(p_delta_time);

    std::cout << "projection" << std::endl;
    projection(p_delta_time);

    endStep();
    std::cout << "update finished" << std::endl;
}
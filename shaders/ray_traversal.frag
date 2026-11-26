#version 430 core

// ================================================================================================
// ======================================== IN/OUT/UNIFORMS =======================================
// ================================================================================================
layout(binding = 0, std430) buffer z_order_grid_data_buff {
    float grid_cell_densities[];
};

uniform float near_plane;
uniform vec3 cam_pos;
uniform mat4 viewproj_inverse;
uniform vec2 inv_screen;

uniform uvec3 grid_size;
uniform vec3 grid_pos;
uniform vec3 grid_cell_dims;

uniform uint n_samples;
uniform vec3 scattering_coefs;
uniform vec3 sun_pos;

in vec2 f_uv;
out vec4 out_color;

// ================================================================================================
// ==================================== RAY GENERATION HELPERS ====================================
// ================================================================================================

// https://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
// A single iteration of Bob Jenkins' One-At-A-Time hashing algorithm.
uint hash(uint x) {
    x += (x << 10u);
    x ^= (x >> 6u);
    x += (x << 3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}

// Compound versions of the hashing algorithm I whipped together.
uint hash(uvec2 v) {
    return hash(v.x ^ hash(v.y));
}
uint hash(uvec3 v) {
    return hash(v.x ^ hash(v.y) ^ hash(v.z));
}
uint hash(uvec4 v) {
    return hash(v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w));
}

// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value below 1.0.
float floatConstruct(uint m) {
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne = 0x3F800000u;      // 1.0 in IEEE binary32

    m &= ieeeMantissa; // Keep only mantissa bits (fractional part)
    m |= ieeeOne;      // Add fractional part to 1.0

    float f = uintBitsToFloat(m); // Range [1:2]
    return f - 1.0;               // Range [0:1]
}

// Pseudo-random value in half-open range [0:1].
float random(float x) {
    return floatConstruct(hash(floatBitsToUint(x)));
}
float random(vec2 v) {
    return floatConstruct(hash(floatBitsToUint(v)));
}
float random(vec3 v) {
    return floatConstruct(hash(floatBitsToUint(v)));
}
float random(vec4 v) {
    return floatConstruct(hash(floatBitsToUint(v)));
}

vec2 screenToCamSpace(vec2 screen_coords) {
    return vec2((screen_coords.x - 0.5) * 2., (0.5 - screen_coords.y) * 2.);
}
vec3 applyTransformation(vec3 vec, float w, mat4 transfo) {
    vec4 temp = transfo * vec4(vec, w);
    return temp.w == 0 ? vec3(temp.x, temp.y, temp.z) : vec3(temp.x, temp.y, temp.z) / temp.w;
}
uvec3 posToCellPos(vec3 _pos) {
    vec3 rel = floor((_pos - grid_pos) / grid_cell_dims);
    return uvec3(max(rel, vec3(0.0)));
}

// ================================================================================================
// ===================================== RAY TRAVERSAL HELPERS ====================================
// ================================================================================================

const float RAY_DIR_EPS = 1.e-8;

// https://www.researchgate.net/publication/2611491_A_Fast_Voxel_Traversal_Algorithm_for_Ray_Tracing
struct VoxelTraversalData {
    uvec3 current_cell_pos, end_cell_pos;
    vec3 t_delta, t_next;
    ivec3 step;

    float last_step_distance;
};
bool intersectRayAABB(vec3 _aabb_min, vec3 _aabb_max, vec3 _ray_origin, vec3 _ray_dir, out float tmin, out float tmax) {
    vec3 inv_dir = 1. / _ray_dir;
    vec3 t0 = (_aabb_min - _ray_origin) * inv_dir;
    vec3 t1 = (_aabb_max - _ray_origin) * inv_dir;

    vec3 t_small = min(t0, t1);
    vec3 t_big = max(t0, t1);

    tmin = max(max(t_small.x, t_small.y), t_small.z);
    tmax = min(min(t_big.x, t_big.y), t_big.z);
    return tmax >= tmin;
}
bool initRayTraversal(vec3 _ray_origin, vec3 _ray_dir, out VoxelTraversalData traversal) {
    vec3 sgn = vec3(greaterThanEqual(_ray_dir, vec3(0.))) * 2. - 1.;
    vec3 tiny = vec3(lessThan(abs(_ray_dir), vec3(RAY_DIR_EPS)));
    _ray_dir = mix(_ray_dir, sgn * RAY_DIR_EPS, tiny);

    vec3 grid_max = grid_pos + vec3(grid_size) * grid_cell_dims;

    float t_enter, t_exit;
    if (!intersectRayAABB(grid_pos, grid_max, _ray_origin, _ray_dir, t_enter, t_exit) || t_exit < 0.)
        return false;
    t_enter = max(t_enter, 0.);

    vec3 start_pos = _ray_origin + t_enter * _ray_dir;
    vec3 end_pos = _ray_origin + t_exit * _ray_dir;

    uvec3 max_cell = grid_size - uvec3(1);
    traversal.current_cell_pos = min(posToCellPos(start_pos), max_cell);
    traversal.end_cell_pos = min(posToCellPos(end_pos), max_cell);

    traversal.step = ivec3(sign(_ray_dir));
    traversal.t_delta = abs(grid_cell_dims / _ray_dir);

    vec3 current_cell_center = grid_pos + vec3(traversal.current_cell_pos) * grid_cell_dims;
    vec3 next_boundary = current_cell_center + vec3(greaterThan(_ray_dir, vec3(0.))) * grid_cell_dims;
    traversal.t_next = (next_boundary - _ray_origin) / _ray_dir;
    traversal.last_step_distance = 0.1;
    return true;
}
bool stepRayTraversal(inout VoxelTraversalData traversal) {
    uint axis;
    if (traversal.t_next.x < traversal.t_next.y && traversal.t_next.x < traversal.t_next.z) {
        axis = 0u;
    } else if (traversal.t_next.y < traversal.t_next.z) {
        axis = 1u;
    } else {
        axis = 2u;
    }

    int next = int(traversal.current_cell_pos[axis]) + traversal.step[axis];
    if (next < 0 || next >= int(grid_size[axis])) {
        return false;
    }

    traversal.current_cell_pos[axis] = uint(next);
    traversal.last_step_distance = traversal.t_next[axis];
    traversal.t_next[axis] += traversal.t_delta[axis];
    traversal.last_step_distance = min(traversal.t_next.x, min(traversal.t_next.y, traversal.t_next.z)) - traversal.last_step_distance;
    return true;
}

// ================================================================================================
// ==================================== RAY Traversal FUNCTIONS ===================================
// ================================================================================================

struct RayData {
    float total_density;
    vec3 total_light;
};
uint cellPosToIndexZorder(uvec3 _pos) {
    uvec3 buff = _pos;
    for (uint dim = 0; dim < 3; dim++) {
        buff[dim] = (buff[dim] | (buff[dim] << 16)) & uint(0xFF0000FF);
        buff[dim] = (buff[dim] | (buff[dim] << 8)) & uint(0x0F00F00F);
        buff[dim] = (buff[dim] | (buff[dim] << 4)) & uint(0xC30C30C3);
        buff[dim] = (buff[dim] | (buff[dim] << 2)) & uint(0x49249249);
    }

    uint res = 0;
    for (uint dim = 0; dim < 3; dim++)
        res |= (buff[dim] << dim);
    return res;
}

float rayDensityTraversal(vec3 _ray_origin, vec3 _ray_direction) {
    float density = 0.;
    VoxelTraversalData traversal;
    if (!initRayTraversal(_ray_origin, _ray_direction, traversal))
        return density;
    do {
        uint cell_index = cellPosToIndexZorder(traversal.current_cell_pos);
        density += grid_cell_densities[cell_index] * traversal.last_step_distance;
    } while (traversal.current_cell_pos != traversal.end_cell_pos && stepRayTraversal(traversal));
    return density;
}

bool rayTraversal(vec3 _ray_origin, vec3 _ray_direction, out VoxelTraversalData traversal, out RayData ray_data) {
    ray_data.total_density = 0.;
    ray_data.total_light = vec3(0.);

    if (!initRayTraversal(_ray_origin, _ray_direction, traversal))
        return false;

    do {
        uint cell_index = cellPosToIndexZorder(traversal.current_cell_pos);

        float density_along_step = grid_cell_densities[cell_index] * traversal.last_step_distance;
        ray_data.total_density += density_along_step;

        vec3 cell_pos = grid_pos + vec3(traversal.current_cell_pos) * grid_cell_dims;
        float sun_total_density = rayDensityTraversal(cell_pos, normalize(sun_pos - cell_pos));
        vec3 transmitted_sun_light = exp(-sun_total_density * scattering_coefs);

        vec3 in_scattered_light = transmitted_sun_light * density_along_step * scattering_coefs;
        vec3 view_ray_transmittance = exp(-ray_data.total_density * scattering_coefs);
        ray_data.total_light += in_scattered_light * view_ray_transmittance;
    } while (traversal.current_cell_pos != traversal.end_cell_pos && stepRayTraversal(traversal));

    return true;
}

void main() {
    out_color = vec4(0., 0., 0., 1.);
    for (uint sample_i = 0; sample_i < n_samples; sample_i++) {
        uint sample_seed = sample_i * 2u;
        vec2 sample_offset = n_samples == 1u ? vec2(0.) : vec2(random(vec3(f_uv, float(sample_seed))), random(vec3(f_uv, float(sample_seed + 1u)))) * inv_screen;
        vec2 xy = screenToCamSpace(f_uv + sample_offset);
        vec3 ray_direction = normalize(applyTransformation(vec3(xy, near_plane), 1., viewproj_inverse) - cam_pos);

        VoxelTraversalData traversal;
        RayData ray_data;
        if (rayTraversal(cam_pos, ray_direction, traversal, ray_data))
            out_color.xyz += ray_data.total_light;
    }
    out_color.xyz /= float(n_samples);
}

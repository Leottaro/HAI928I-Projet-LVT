#version 430 core
const float FLT_MAX = 3.402823466e+38;

// ================================================================================================
// ======================================== IN/OUT/UNIFORMS =======================================
// ================================================================================================
layout(binding = 0, std430) buffer z_order_grid_data_buff {
    float grid_cell_densities[];
};

uniform uvec3 grid_size;
uniform vec3 grid_pos;
uniform vec3 grid_cell_dims;

uniform float near_plane;
uniform vec3 cam_pos;
uniform mat4 viewproj_inverse;

uniform uint n_samples;
uniform vec2 inv_screen;

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

// ================================================================================================
// ===================================== RAY TRAVERSAL HELPERS ====================================
// ================================================================================================

struct RayTraversalInfo {
    uvec3 current_cell_pos, end_cell_pos;
    vec3 tdelta, t;
    ivec3 step;
};
bool intersectRayAABB(vec3 aabb_min, vec3 aabb_max, inout vec3 origin, inout vec3 direction, out float tmin, out float tmax) {
    vec3 delta_min = aabb_min - origin;
    vec3 delta_max = aabb_max - origin;

    float t0, t1, ttemp;
    int f0, f1, ftemp;

    // X slab: -X=0, +X=3
    t0 = delta_min.x / direction.x;
    t1 = delta_max.x / direction.x;
    f0 = 0;
    f1 = 3; // -X, +X
    if(t0 > t1) {
        ttemp = t0;
        t0 = t1;
        t1 = ttemp;

        ftemp = f0;
        f0 = f1;
        f1 = ftemp;
    }
    tmin = t0;
    tmax = t1;

    // Y slab: -Y=1, +Y=4
    float tmin_tmp = delta_min.y / direction.y;
    float tmax_tmp = delta_max.y / direction.y;
    int fmin_tmp = 1, fmax_tmp = 4; // -Y, +Y
    if(tmin_tmp > tmax_tmp) {
        ttemp = tmin_tmp;
        tmin_tmp = tmax_tmp;
        tmax_tmp = ttemp;

        ftemp = fmin_tmp;
        fmin_tmp = fmax_tmp;
        fmax_tmp = ftemp;
    }

    if(tmax_tmp < tmin || tmin_tmp > tmax)
      return false;
    if(tmin_tmp > tmin) {
        tmin = tmin_tmp;
    }
    if(tmax_tmp < tmax) {
        tmax = tmax_tmp;
    }

    // Z slab: -Z=2, +Z=5
    tmin_tmp = delta_min.z / direction.z;
    tmax_tmp = delta_max.z / direction.z;
    fmin_tmp = 2;
    fmax_tmp = 5; // -Z, +Z
    if(tmin_tmp > tmax_tmp) {
        ttemp = tmin_tmp;
        tmin_tmp = tmax_tmp;
        tmax_tmp = ttemp;

        ftemp = fmin_tmp;
        fmin_tmp = fmax_tmp;
        fmax_tmp = ftemp;
    }

    if(tmax_tmp < tmin || tmin_tmp > tmax)
      return false;
    if(tmin_tmp > tmin) {
        tmin = tmin_tmp;
    }
    if(tmax_tmp < tmax) {
        tmax = tmax_tmp;
    }

    return true;
}
uvec3 posToCellPos(vec3 _pos) {
    vec3 rel = floor((_pos - grid_pos) / grid_cell_dims);
    return uvec3(max(rel, vec3(0.0)));
}
bool initRayTrace(vec3 _ray_origin, vec3 _ray_direction, out RayTraversalInfo _infos) {
    float t1, t2;
    if(!intersectRayAABB(grid_pos, grid_pos + vec3(grid_size) * grid_cell_dims, _ray_origin, _ray_direction, t1, t2) || t2 < 0.)
      return false;

    vec3 start = t1 < 0. ? _ray_origin : _ray_origin + t1 * _ray_direction;
    vec3 end = _ray_origin + t2 * _ray_direction;
    _infos.current_cell_pos = posToCellPos(start);
    _infos.end_cell_pos = posToCellPos(end);

    for(int axis = 0; axis < 3; axis++) {
        _infos.current_cell_pos[axis] = min(_infos.current_cell_pos[axis], grid_size[axis] - 1);
        _infos.end_cell_pos[axis] = min(_infos.end_cell_pos[axis], grid_size[axis] - 1);

        if(_ray_direction[axis] == 0.) {
            _infos.step[axis] = 0;
            _infos.tdelta[axis] = _infos.t[axis] = FLT_MAX;
        } else {
            _infos.step[axis] = _ray_direction[axis] > 0. ? 1 : -1;
            _infos.tdelta[axis] = abs(grid_cell_dims[axis] / _ray_direction[axis]);

            float cell_min = grid_pos[axis] + _infos.current_cell_pos[axis] * grid_cell_dims[axis];
            float next_boundary = cell_min + (_ray_direction[axis] > 0. ? grid_cell_dims[axis] : 0.);
            _infos.t[axis] = (next_boundary - start[axis]) / _ray_direction[axis];
        }
    }

    return true;
}
bool stepRayTrace(inout RayTraversalInfo _infos) {
    uint axis;
    if(_infos.t.x < _infos.t.y && _infos.t.x < _infos.t.z)
      axis = 0;
    else if(_infos.t.y < _infos.t.z)
      axis = 1;
    else
      axis = 2;

    int next = int(_infos.current_cell_pos[axis]) + _infos.step[axis];
    if(next < 0 || next >= int(grid_size[axis]))
        return false;

    _infos.current_cell_pos[axis] = uint(next);
    _infos.t[axis] += _infos.tdelta[axis];
    return true;
}

// ================================================================================================
// ==================================== RAY TRAVERSAL FUNCTIONS ===================================
// ================================================================================================

const vec3 WATER_COLOR = vec3(0.32, 0.64, 0.85);
struct RayData {
    float max_density;
};
void rayInit(inout RayData _ray_data) {
    _ray_data.max_density = 0.;
}
void rayStep(inout RayData _ray_data, uvec3 _pos, uint _cell_index) {
    _ray_data.max_density = max(_ray_data.max_density, grid_cell_densities[_cell_index]);
}
bool rayStopCondition(inout RayData _ray_data, uvec3 _pos, uint _cell_index) {
    return false;
}
vec4 rayColor(in RayData _ray_data) {
    return vec4(WATER_COLOR, _ray_data.max_density);
}
uint cellPosToIndexZorder(uvec3 _pos) {
    uvec3 buff = _pos;
    for(uint dim = 0; dim < 3; dim++) {
        buff[dim] = (buff[dim] | (buff[dim] << 16)) & uint(0xFF0000FF);
        buff[dim] = (buff[dim] | (buff[dim] << 8)) & uint(0x0F00F00F);
        buff[dim] = (buff[dim] | (buff[dim] << 4)) & uint(0xC30C30C3);
        buff[dim] = (buff[dim] | (buff[dim] << 2)) & uint(0x49249249);
    }

    uint res = 0;
    for(uint dim = 0; dim < 3; dim++) res |= (buff[dim] << dim);
    return res;
}

void main() {
    // generate ray direction
    vec2 sample_offset = n_samples <= 1 ? vec2(0.) : vec2(random(vec3(f_uv, 0.)), random(vec3(f_uv, 1.))) * inv_screen;
    vec2 xy = screenToCamSpace(f_uv + sample_offset);
    vec3 ray_direction = normalize(applyTransformation(vec3(xy, near_plane), 1., viewproj_inverse) - cam_pos);

    RayTraversalInfo infos;
    if(!initRayTrace(cam_pos, ray_direction, infos)) {
        out_color = vec4(0.);
        return;
    }

    uint cell_index = cellPosToIndexZorder(infos.current_cell_pos);

    RayData ray_data;
    rayInit(ray_data);
    rayStep(ray_data, infos.current_cell_pos, cell_index);

    while(infos.current_cell_pos != infos.end_cell_pos && stepRayTrace(infos)) {
        cell_index = cellPosToIndexZorder(infos.current_cell_pos);
        rayStep(ray_data, infos.current_cell_pos, cell_index);
    }

    out_color = rayColor(ray_data);
}

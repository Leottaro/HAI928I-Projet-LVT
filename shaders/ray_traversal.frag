#version 460 core

// ================================================================================================
// ======================================== IN/OUT/UNIFORMS =======================================
// ================================================================================================

uniform float near_plane;
uniform vec3 cam_pos;
uniform mat4 viewproj_inverse;
uniform vec2 inv_screen;

uniform sampler3D grid_cell_densities;
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

// Source - https://stackoverflow.com/a/17479300
// Posted by Spatial
// Retrieved 2026-11-27, License - CC BY-SA 3.0
// A single iteration of Bob Jenkins' One-At-A-Time hashing algorithm.
uint hash( uint x ) {
    x += ( x << 10u );
    x ^= ( x >>  6u );
    x += ( x <<  3u );
    x ^= ( x >> 11u );
    x += ( x << 15u );
    return x;
}
// Compound versions of the hashing algorithm I whipped together.
uint hash( uvec2 v ) { return hash( v.x ^ hash(v.y)                         ); }
uint hash( uvec3 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z)             ); }
uint hash( uvec4 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w) ); }
// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value below 1.0.
float floatConstruct( uint m ) {
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                          // Add fractional part to 1.0

    float  f = uintBitsToFloat( m );       // Range [1:2]
    return f - 1.0;                        // Range [0:1]
}
// Pseudo-random value in half-open range [0:1].
float random( float x ) { return floatConstruct(hash(floatBitsToUint(x))); }
float random( vec2  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
float random( vec3  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
float random( vec4  v ) { return floatConstruct(hash(floatBitsToUint(v))); }



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

const float FLT_MAX = 3.40282347e38f;

// https://www.researchgate.net/publication/2611491_A_Fast_Voxel_Traversal_Algorithm_for_Ray_Tracing
struct VoxelTraversalData {
    uvec3 current_cell_pos;
    vec3 t_delta, t_max;
    uvec3 step;

    vec3 origin, dir, inv_dir, sgn_dir;
    uint stepped_axis;
    float t_in, t_out;
};
uint getStepAxis(in vec3 t_max) {
    return t_max.x < t_max.y && t_max.x < t_max.z  ? 0u
         : t_max.x >= t_max.y && t_max.y < t_max.z ? 1u
                                                   : 2u;
}
bool stepRayTraversal(inout VoxelTraversalData traversal) {
    traversal.stepped_axis = getStepAxis(traversal.t_max);

    traversal.t_in = traversal.t_out;
    traversal.t_out = traversal.t_max[traversal.stepped_axis];

    traversal.current_cell_pos[traversal.stepped_axis] += traversal.step[traversal.stepped_axis];
    traversal.t_max[traversal.stepped_axis] += traversal.t_delta[traversal.stepped_axis];
    

    // vec3 mask = vec3(0);
    // mask[traversal.stepped_axis] = 1;
    
    // normal
    // vec3 normal = -mask * traversal.sgn_dir;
    
    // depth
    // float depth = (t_start + t_inside) * length(traversal.dir);
    
    // uv
    // vec3 hit = traversal.origin + traversal.dir * t_inside;
    // vec3 local_hit = hit - traversal.current_cell_pos;
    // vec2 uv;
    // if (traversal.stepped_axis == 0) {
    //     uv = local_hit.yz;
    // } else if (traversal.stepped_axis == 1) {
    //     uv = local_hit.xz;
    // } else {
    //     uv = local_hit.xy;
    // }

    return !any(greaterThanEqual(traversal.current_cell_pos, grid_size));
}
bool initRayTraversal(vec3 _ray_origin, vec3 _ray_dir, out VoxelTraversalData traversal) {
    traversal.origin = _ray_origin;
    traversal.dir = _ray_dir;
    traversal.inv_dir = clamp(1. / _ray_dir, vec3(-FLT_MAX), vec3(FLT_MAX));
    traversal.sgn_dir = sign(traversal.inv_dir);

    // Grid AABB intersection
    const vec3 grid_max = (grid_pos+grid_size*grid_cell_dims);
    vec3 t1 = (grid_pos - _ray_origin) * traversal.inv_dir;
    vec3 t2 = (grid_max - _ray_origin) * traversal.inv_dir;

    vec3 t_mins = min(t1, t2);
    vec3 t_maxs = max(t1, t2);

    float t_enter = max(max(t_mins.x, t_mins.y), t_mins.z);
    float t_exit = min(min(t_maxs.x, t_maxs.y), t_maxs.z);
    if (t_enter > t_exit) 
        return false;
    
    traversal.stepped_axis = t_enter < 0. ? 3 : uint(dot(vec3(0, 1, 2), vec3(equal(t_mins, vec3(t_enter)))));
    t_enter = max(t_enter, 0);

    // ray traversal initialisation
    vec3 enter_pos = _ray_origin + _ray_dir * t_enter;
    vec3 enter_pos_grid_ref = (enter_pos - grid_pos) / grid_cell_dims;
    traversal.current_cell_pos = uvec3(clamp(floor(enter_pos_grid_ref), vec3(0), vec3(grid_size - 1)));
    traversal.t_max = (traversal.current_cell_pos + 0.5 * (1 + traversal.sgn_dir) - enter_pos_grid_ref) * traversal.inv_dir;

    traversal.step = uvec3(ivec3(traversal.sgn_dir));
    traversal.t_delta = traversal.inv_dir * traversal.sgn_dir;
    
    traversal.stepped_axis = getStepAxis(traversal.t_max);
    traversal.t_in = 0.;
    traversal.t_out = traversal.t_max[traversal.stepped_axis];

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
        float density_along_step = texture(grid_cell_densities, vec3(traversal.current_cell_pos) / vec3(grid_size)).r * (traversal.t_out - traversal.t_in);
        density += density_along_step;
    } while (stepRayTraversal(traversal));
    return density;
}

bool rayTraversal(vec3 _ray_origin, vec3 _ray_direction, out VoxelTraversalData traversal, out RayData ray_data) {
    ray_data.total_density = 0.;
    ray_data.total_light = vec3(0.);

    if (!initRayTraversal(_ray_origin, _ray_direction, traversal))
        return false;

    do {
        // TODO: reflection, refraction, etc...

        float density_along_step = texture(grid_cell_densities, vec3(traversal.current_cell_pos) / vec3(grid_size)).r * (traversal.t_out - traversal.t_in);
        ray_data.total_density += density_along_step;

        // TODO: sample multiple point along view ray
        vec3 cell_pos = grid_pos + vec3(traversal.current_cell_pos) * grid_cell_dims;
        float sun_total_density = rayDensityTraversal(cell_pos, normalize(sun_pos - cell_pos));
        vec3 transmitted_sun_light = exp(-sun_total_density * scattering_coefs);

        vec3 in_scattered_light = transmitted_sun_light * density_along_step * scattering_coefs;
        vec3 view_ray_transmittance = exp(-ray_data.total_density * scattering_coefs);
        ray_data.total_light += in_scattered_light * view_ray_transmittance;
    } while (stepRayTraversal(traversal));

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

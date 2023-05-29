#version 430 core

#define WORK_GROUP_DIM 16

layout (local_size_x = WORK_GROUP_DIM, local_size_y = WORK_GROUP_DIM) in;

layout (binding = 0, rgba32f) uniform image2D u_initial_spectrum; // h_0(k)
layout (binding = 1, rgba32f) uniform image2D u_vertical_displacement; // h_t(k)
layout (binding = 2, rgba32f) uniform image2D u_dx_displacement;
layout (binding = 3, rgba32f) uniform image2D u_dz_displacement;

uniform int u_resolution;
uniform int u_ocean_size;
uniform float u_choppiness;
uniform float u_time;

const float g = 9.81; // gravity 
const float PI = 3.14159265358979323846264; // Life of π


// COMPLEX OPERATIONS
vec2 prod(const vec2 a, const vec2 b){
    return vec2(a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x);
} 
vec2 conj(const vec2 a){
    return vec2(a.x, -a.y);
}
vec2 euler(const float x){
    return vec2(cos(x), sin(x));
}

float omega(float k)
{
    return sqrt(g * k);
}

void main(void)
{
    ivec2 pixel_coord = ivec2(gl_GlobalInvocationID.xy);
    float n = (pixel_coord.x < 0.5f * u_resolution) ? pixel_coord.x : pixel_coord.x - u_resolution;
	float m = (pixel_coord.y < 0.5f * u_resolution) ? pixel_coord.y : pixel_coord.y - u_resolution;
    
    // vec2 wave_vector = (2.f * PI * (pixel_coord - (u_resolution>>1))) / u_ocean_size;
    // vec2 wave_vector = (2.f * PI * vec2(n, m)) / u_ocean_size;
    // vec2 wave_vector = (2.f * PI * ((pixel_coord + (u_resolution>>1))%(u_resolution))) / u_ocean_size;

    ivec2 center_offset = ivec2(u_resolution >> 1);
    ivec2 wrapped_coord = (pixel_coord + center_offset) % u_resolution - center_offset;
    vec2 wave_vector = (2.f * PI * wrapped_coord) / u_ocean_size; 


    float k = length(wave_vector);

    float phase = omega(k) * u_time;

    vec2 h0 = imageLoad(u_initial_spectrum, pixel_coord).rg;
    ivec2 inv_pixel_coord = (u_resolution - pixel_coord);
    vec2 h0_est = conj(imageLoad(u_initial_spectrum, inv_pixel_coord).rg);
    // vec2 h0_est = imageLoad(u_initial_spectrum, pixel_coord).ba;

    vec2 h = prod(h0, euler(phase)) + prod(h0_est, euler(-phase));
    vec2 nx = prod(vec2(0,1), h) * wave_vector.x;
    vec2 nz = prod(vec2(0,1), h) * wave_vector.y;
     
    k = max(k, 0.1);
    vec2 Dz = -nz/k * u_choppiness;
    vec2 Dx = -nx/k * u_choppiness;
    
    // vec2 Dx = (k == 0) ? vec2(0) : prod(vec2(0,-1), h) * wave_vector.x/k * u_choppiness;
    // vec2 Dz = (k == 0) ? vec2(0) : prod(vec2(0,-1), h) * wave_vector.y/k * u_choppiness;
    
    // imageStore(u_vertical_displacement, pixel_coord, vec4(h, 0.f, 0.f));
    // imageStore(u_dx_displacement, pixel_coord, vec4(Dx, 0.f, 0.f));
    // imageStore(u_dz_displacement, pixel_coord, vec4(Dz,  0.f, 0.f));
    imageStore(u_vertical_displacement, pixel_coord, vec4(h, 0.f, 0.f));
    imageStore(u_dx_displacement, pixel_coord, vec4(Dx,nx));
    imageStore(u_dz_displacement, pixel_coord, vec4(Dz,nz));

}
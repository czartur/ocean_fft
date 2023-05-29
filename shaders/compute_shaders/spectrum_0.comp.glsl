#version 430 core

#define WORK_GROUP_DIM 16

layout (local_size_x = WORK_GROUP_DIM, local_size_y = WORK_GROUP_DIM) in;

layout (binding = 0, rgba32f) writeonly uniform image2D u_initial_spectrum; //h_0(k)
layout (binding = 1, rgba32f) readonly uniform image2D u_gaussian_noise; 

uniform int u_resolution; // resolution
uniform int u_ocean_size; // dimension
uniform float u_amplitude; // amplitude
uniform vec2 u_wind; // wind direction

const float g = 9.81; // gravity 
const float PI = 3.14159265358979323846264; // Life of π
// const float l = u_ocean_size / 2000;
const float l = 1.5;

float philips(in vec2 wave_vector){
    float V = length(u_wind);
    float Lp = V*V/g;
    float k = length(wave_vector);
    k = max(k, 0.1); 
    // return clamp(sqrt(
    //         u_amplitude
    //         *pow(dot(normalize(wave_vector), normalize(u_wind)), 2.0)
    //         *exp(-1.f/(pow(k*Lp,2.0)))
    //         // *exp(-1.f*pow(k*l,2.0))
    //     )/(k*k), -4000, 4000);
    return clamp(sqrt(
            u_amplitude
            *pow(dot(normalize(wave_vector), normalize(u_wind)), 2.0)
            *exp(-1.f/(pow(k*Lp,2.0)))
            *exp(-1.f*pow(k*l,2.0))
        )/(k*k), 0, 4000);  
}

// COMPLEX OPERATIONS
vec2 conj(const vec2 a){
    return vec2(a.x, -a.y);
}

// debug
bool isnan( float val )
{
  return ( val < 0.0 || 0.0 < val || val == 0.0 ) ? false : true;
}

void main(void)
{

    ivec2 pixel_coord = ivec2(gl_GlobalInvocationID.xy);
    float n = (pixel_coord.x < 0.5f * u_resolution) ? pixel_coord.x : pixel_coord.x - u_resolution;
	float m = (pixel_coord.y < 0.5f * u_resolution) ? pixel_coord.y : pixel_coord.y - u_resolution;

    // vec2 wave_vector = (2.f * PI * vec2(n, m)) / u_ocean_size;
    // vec2 wave_vector = (2.f * PI * ((pixel_coord + (u_resolution>>1))%(u_resolution))) / u_ocean_size;
    
    ivec2 center_offset = ivec2(u_resolution >> 1);
    ivec2 wrapped_coord = (pixel_coord + center_offset) % u_resolution - center_offset;
    vec2 wave_vector = (2.f * PI * wrapped_coord) / u_ocean_size; 

    vec2 E_p = imageLoad(u_gaussian_noise, pixel_coord).ra;
    // vec2 E_n = imageLoad(u_gaussian_noise, pixel_coord).ba;

    float vp = philips(wave_vector)/sqrt(2.0);
    float vn = philips(-wave_vector)/sqrt(2.0);

    vec2 h = E_p*vp;
    vec2 h_est = conj(E_p*vn);
    
    imageStore(u_initial_spectrum, pixel_coord, vec4(h, h_est));
    

    // NAN TEST
    // vec3 color = vec3(1,0,0);
    // if(isnan(h.x + h.y)) color = vec3(0,1,0);
    // imageStore(u_initial_spectrum, pixel_coord, vec4(color, 0.f));
}
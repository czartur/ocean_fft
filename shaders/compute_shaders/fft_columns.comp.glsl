#version 430 core

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, rgba32f) uniform readonly image2D u_input;
layout(binding = 1, rgba32f) uniform writeonly image2D u_output;

// REF: http://wwwa.pikara.ne.jp/okojisan/otfft-en/stockham2.html

uniform int u_resolution; // N
uniform int u_stride; // s
uniform int u_count; // n

const float PI = 3.14159265358979323846264; // Life of π

// COMPLEX OPERATIONS
vec2 prod(const vec2 a, const vec2 b){
    return vec2(a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x);
} 
vec2 euler(const float x){
    return vec2(cos(x), sin(x));
}

void main()
{
    int row = int(gl_GlobalInvocationID.x);
    int col = int(gl_GlobalInvocationID.y);
    int q = col % u_stride;
    int p = (col - q) / u_stride;

    vec4 a = imageLoad(u_input, ivec2(col, row));
    vec4 b = imageLoad(u_input, ivec2(col + (u_resolution>>1), row));

    vec2 wp = euler(p * 2.f * PI / u_count);
    vec4 fadd = a + b;
    vec4 fsub = vec4(prod(a.xy-b.xy, wp), prod(a.zw-b.zw, wp));
    
    p<<=1;
    imageStore(u_output, ivec2(q + u_stride*p, row), fadd);
    imageStore(u_output, ivec2(q + u_stride*(p+1), row), fsub);

}

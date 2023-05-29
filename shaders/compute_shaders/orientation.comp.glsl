#version 430 core

#define WORK_GROUP_DIM 16

layout (local_size_x = WORK_GROUP_DIM, local_size_y = WORK_GROUP_DIM) in;

layout (binding = 0, rgba32f) uniform image2D u_input; 
layout (binding = 1, rgba32f) uniform image2D u_output; 

uniform int u_resolution;

void main(void)
{
    ivec2 pixel_coord = ivec2(gl_GlobalInvocationID.xy);
    vec4 img = imageLoad(u_input, pixel_coord); 

    if(pixel_coord.x < (u_resolution>>1)) pixel_coord.x += u_resolution>>1;
    else pixel_coord.x -= u_resolution>>1;

    if(pixel_coord.y < (u_resolution>>1)) pixel_coord.y += u_resolution>>1;
    else pixel_coord.y -= u_resolution>>1;
    
    imageStore(u_output, pixel_coord, img);    
}
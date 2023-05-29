#version 430 core

// Vertex shader - this code is executed for every vertex of the shape

// const float u_ocean_size = 1024;

// Inputs coming from VBOs
layout (location = 0) in vec3 vertex_position; // vertex position in local space (x,y,z)
layout (location = 1) in vec3 vertex_normal;   // vertex normal in local space   (nx,ny,nz)
layout (location = 2) in vec3 vertex_color;    // vertex color      (r,g,b)
layout (location = 3) in vec2 vertex_uv;       // vertex uv-texture (u,v)

// Output variables sent to the fragment shader
out struct data
{
    vec3 position; // vertex position in world space
    vec3 normal;   // normal position in world space
    vec3 color;    // vertex color
    vec2 uv;       // vertex uv
} fragment;

// Uniform variables expected to receive from the C++ program
uniform mat4 model; // Model affine transform matrix associated to the current shape
uniform mat4 view;  // View matrix (rigid transform) of the camera
uniform mat4 projection; // Projection (perspective or orthogonal) matrix of the camera
uniform mat4 modelNormal; // Model without scaling used for the normal. modelNormal = transpose(inverse(model))

// uniform float t;
// uniform float lambda = 5.0f;
// uniform float frequency = 2.0f;

uniform sampler2D u_displacement_map;
uniform sampler2D u_normal_map;
uniform int u_resolution;

// Deformer function for position
vec3 deformer(vec3 p0)
{
	return p0 + vec3(texture(u_displacement_map, vertex_uv).rgb / float(u_resolution * u_resolution));
}

// Deformer function for the normal
vec3 deformer_normal()
{
	return texture(u_normal_map, vertex_uv).xyz;
}

out float dy;

void main()
{

	vec4 position = model * vec4(deformer(vertex_position), 1.0);
	vec4 normal = modelNormal * vec4(deformer_normal(), 0.0);

	// The projected position of the vertex in the normalized device coordinates:
	vec4 position_projected = projection * view * position;

	// Fill the parameters sent to the fragment shader
	fragment.position = position.xyz;
	fragment.normal = normal.xyz;
	fragment.color = vertex_color;
	fragment.uv = vertex_uv;

	// dy = texture(u_displacement_map, vertex_uv).g;
	dy = deformer(vertex_position).y;

	// gl_Position is a built-in variable which is the expected output of the vertex shader
	gl_Position = position_projected; // gl_Position is the projected vertex position (in normalized device coordinates)
}

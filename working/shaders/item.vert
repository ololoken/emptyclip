#version 300 es

layout(location = 0) in vec2 vertex_position;
layout(location = 1) in vec2 vertex_uv;

uniform mat4 view_projection_transform;
uniform float pos_z;

smooth out vec3 world_position;
out vec3 world_normal;
out vec2 texture_coord;

void main() {
	vec4 vec4_vertex_position = vec4(vertex_position, pos_z, 1);

	world_position = vec3(vertex_position, pos_z);
	world_normal = vec3(0, 0, 1);
	texture_coord = vertex_uv;
	gl_Position = view_projection_transform * vec4_vertex_position;
}

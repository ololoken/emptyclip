#version 330 core

layout(location = 0) in vec2 vertex_position;
layout(location = 1) in vec2 vertex_uv;

uniform mat4 view_projection_transform;
out vec2 texture_coord;

void main(void) {
	texture_coord = vertex_uv;
	gl_Position = view_projection_transform * vec4(vertex_position, 0, 1);
}

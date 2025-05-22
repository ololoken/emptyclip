#version 300 es

layout(location = 0) in vec2 vertex_position;

uniform mat4 view_projection_transform;

void main(void) {
	gl_Position = view_projection_transform * vec4(vertex_position, 0, 1);
}

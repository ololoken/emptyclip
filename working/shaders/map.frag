#version 330 core

#define MAX_LIGHTS 10

uniform sampler2D sampler0;
uniform sampler2D sampler1;
uniform vec4 ambient_light;
uniform vec4 color;
uniform vec4 fog_color;

uniform int light_count;
uniform struct light {
	vec4 color;
	vec3 position;
	vec3 attenuation;
} lights[MAX_LIGHTS];

smooth in vec3 world_position;
in vec3 world_normal;
in vec2 texture_coord;
out vec4 out_color;

void main() {

	// Get light color from ambient and mixed light framebuffer
	vec4 light_color = ambient_light + texelFetch(sampler1, ivec2(gl_FragCoord.xy), 0);
	light_color.a = 1;

	// Calculate Lambertian lighting
	for(int i = 0; i < light_count; i++) {

		// Get direction to light
		vec3 light_direction = lights[i].position - world_position;
		float light_distance = length(light_direction);

		// Normalize
		light_direction /= light_distance;

		// Calculate diffuse color
		vec4 diffuse_light = lights[i].color * max(dot(world_normal, light_direction), 0.0);
		float attenuation = 1.0 / (lights[i].attenuation.x + lights[i].attenuation.y * light_distance + lights[i].attenuation.z * light_distance * light_distance);

		// Add lights up
		light_color += diffuse_light * attenuation;
	}

	// Get color from texture
	vec4 texture_color = texture(sampler0, texture_coord);

	// Final color
	out_color = color * texture_color * light_color;

	// Apply fog
	if(fog_color.a > 0 && world_position.z < 0) {
		const float LOG2 = 1.442695;
		float fog_factor = clamp(exp2(-fog_color.a * fog_color.a * world_position.z * world_position.z * LOG2), 0.0, 1.0);
		out_color.rgb = mix(fog_color.rgb, out_color.rgb, fog_factor);
	}
}

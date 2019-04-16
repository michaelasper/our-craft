#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

uniform mat4 projection;
uniform mat4 view;

in vec4 vs_light_direction[];
flat out vec4 normal;
flat out vec4 world_norm;

in vec4 vs_world_pos[];


out vec4 light_direction;
out vec4 world_position;
void main()
{

	vec3 w_p1 = vec3(vs_world_pos[0]);
    vec3 w_p2 = vec3(vs_world_pos[1]);
    vec3 w_p3 = vec3(vs_world_pos[2]);
    world_norm = vec4(normalize(cross(w_p2 - w_p1, w_p3 - w_p1)), 0.0);

	vec3 p1 = gl_in[0].gl_Position.xyz;
	vec3 p2 = gl_in[1].gl_Position.xyz;
	vec3 p3 = gl_in[2].gl_Position.xyz;

	vec3 edge1 = p2 - p1;
	vec3 edge2 = p3 - p1;

	normal = vec4(normalize(-cross(edge1,edge2)), 0.0);

	for (int n = 0; n < gl_in.length(); n++) {
		light_direction = vs_light_direction[n];
		world_position = vs_world_pos[n];
		gl_Position = projection  * gl_in[n].gl_Position;
		EmitVertex();
	}
	EndPrimitive();
}
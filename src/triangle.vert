#version 330 core
in vec4 vertex_position;

uniform mat4 view;
uniform vec4 light_position;

out vec4 vs_light_direction;
out vec4 vs_world_pos;
void main()
{
    vs_world_pos = vertex_position;
	gl_Position = view * vertex_position;
	vs_light_direction = view * (vertex_position - light_position);
}
#version 330 core
flat in vec4 normal;
flat in vec4 world_norm;
in vec4 light_direction;
out vec4 fragment_color;
void main()
{
    vec4 color = vec4 (normal.x, normal.y,normal.z, 0.f);
    vec4 base_col = clamp(world_norm * world_norm, 0.0, 1.0);
    float intensity = clamp(-dot(light_direction, color), 0.0, 1.0);
    fragment_color = clamp(intensity * base_col, 0.0, 1.0);
    fragment_color.a = 1.0;
}
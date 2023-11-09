#version 330 core

uniform mat4 u_mvp;

layout(location = 0) in vec4 a_color;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_position;
layout(location = 3) in vec3 a_tangent;
layout(location = 4) in vec2 a_texCoord;

out vec4 color;
out vec2 texCoord;

void main(void)
{
    color = a_color;
    texCoord = a_texCoord;

    gl_Position = u_mvp * a_position;
}

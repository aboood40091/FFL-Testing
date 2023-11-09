#version 330 core

uniform int u_mode;
uniform vec3 u_const1;
uniform vec3 u_const2;
uniform vec3 u_const3;

struct PS_PUSH_DATA
{
    uint alphaFunc;
    float alphaRef;
};

uniform PS_PUSH_DATA PS_PUSH;

uniform sampler2D s_texture;

in vec4 color;
in vec2 texCoord;

out vec4 FragColor;

void main(void)
{
    // I honestly don't know what to do with the vertex color

    if (u_mode == 0)
        FragColor = vec4(u_const1, 1.0);

    else if (u_mode == 1)
        FragColor = texture(s_texture, texCoord);

    else if (u_mode == 2)
    {
        vec4 textureColor = texture(s_texture, texCoord);
        FragColor = vec4(
            u_const1 * textureColor.r +
            u_const2 * textureColor.g +
            u_const3 * textureColor.b,
            textureColor.a
        );
    }
    else if (u_mode == 3)
    {
        vec4 textureColor = texture(s_texture, texCoord);
        FragColor = vec4(
            u_const1 * textureColor.r,
            textureColor.r
        );
    }
    else if (u_mode == 4)
    {
        vec4 textureColor = texture(s_texture, texCoord);
        FragColor = vec4(
            u_const1 * textureColor.g,
            textureColor.r
        );
    }
    else if (u_mode == 5)
    {
        vec4 textureColor = texture(s_texture, texCoord);
        FragColor = vec4(
            u_const1 * textureColor.r,
            1.0
        );
    }
}

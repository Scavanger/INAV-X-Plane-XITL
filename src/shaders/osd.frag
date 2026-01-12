#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2DArray textureArray;
uniform int layer;

void main()
{
    FragColor = texture(textureArray, vec3(TexCoord, layer));
}

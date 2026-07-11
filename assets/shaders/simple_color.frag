#version 420 core

out vec4 FragColor;

uniform vec3 uColor; // highlight color (e.g. yellow)

void main()
{
    FragColor = vec4(uColor, 1.0);
}

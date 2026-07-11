#version 420 core

layout(location = 0) in vec3 aPos;  // Vertex position

uniform mat4 WVP; // World-View-Projection matrix

void main()
{
    gl_Position = WVP * vec4(aPos, 1.0);
}

#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;    // Added: Vertex Normals
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 FragPos;                         // Sent to fragment shader
out vec3 Normal;                          // Sent to fragment shader

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // 1. Calculate the fragment's position in world space
    FragPos = vec3(model * vec4(aPos, 1.0));
    
    // 2. Transform normals to world space using a Normal Matrix
    // This prevents non-uniform scaling from distorting the normal vectors
    Normal = mat3(transpose(inverse(model))) * aNormal;  
    
    // 3. Pass through texture coordinates
    TexCoords = aTexCoords;
    
    // 4. Final clip space position
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
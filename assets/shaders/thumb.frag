#version 330 core
out vec4 FragColor;

in vec2 TexCoords; // Don't forget to pass this from the vertex shader!
in vec3 Normal;

uniform sampler2D texture_diffuse1; // Or whatever your Model class binds to

void main() {
    // 1. Lighting Math
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.5));
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 lighting = vec3(diff + 0.2); // Diffuse + Ambient
    
    // 2. Sample the Texture
    vec4 texColor = texture(texture_diffuse1, TexCoords);
    
    // 3. Combine (Multiply)
    FragColor = vec4(texColor.rgb * lighting, texColor.a);
}
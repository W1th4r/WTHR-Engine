#version 330 core
out vec4 FragColor;
//AA
// --- Material System Uniforms ---
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

uniform bool useTexture;
uniform bool useColor;
uniform bool useLighting;
uniform bool useSpotLight; // Toggle to switch flashlight on/off dynamically
uniform vec4 uColor;
uniform float materialShininess;

// --- Light Structure Definitions ---
struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float linear;
    float quadratic;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
  
    float constant;
    float linear;
    float quadratic;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;       
};

#define MAX_POINT_LIGHTS 16

// --- Varying Inputs from Vertex Shader ---
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// --- Light & Camera Uniforms ---
uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int activePointLightCount; // Dynamically set via EnTT loop count
uniform SpotLight spotLight;

// --- Function Prototypes ---
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 baseColor, vec3 specMap);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor, vec3 specMap);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor, vec3 specMap);

void main()
{    
    // 1. EVALUATE BASE COLOR (Handles texturing, flat tinting, or blending both)
    vec4 baseColor = vec4(1.0);
    
    if (useTexture && useColor) {
        baseColor = texture(texture_diffuse1, TexCoords) * uColor;
    } else if (useTexture) {
        baseColor = texture(texture_diffuse1, TexCoords);
    } else if (useColor) {
        baseColor = uColor;
    }
    
    // Early transparency discard alpha mask
    if (baseColor.a < 0.1) discard;

    // 2. EVALUATE SPECULAR PROPERTIES
    vec3 specularMap = useTexture ? vec3(texture(texture_specular1, TexCoords)) : vec3(0.5);

    // 3. LIGHTING PIPELINE CORRIDOR
    if (useLighting) {
        vec3 norm = normalize(Normal);
        vec3 viewDir = normalize(viewPos - FragPos);
        
        // Phase 1: Directional Lighting (The Sun)
        vec3 result = CalcDirLight(dirLight, norm, viewDir, vec3(baseColor), specularMap);
        
        // Phase 2: Dynamic Point Lights Loop
        int totalPoints = clamp(activePointLightCount, 0, MAX_POINT_LIGHTS);
        for(int i = 0; i < totalPoints; i++) {
            result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, vec3(baseColor), specularMap);    
        }
        
        // Phase 3: Optional Spot Light (Flashlight / Headlamp)
        if (useSpotLight) {
            result += CalcSpotLight(spotLight, norm, FragPos, viewDir, vec3(baseColor), specularMap);    
        }
        
        FragColor = vec4(result, baseColor.a);
    } 
    else {
        // Unlit path: Fallback for mesh light representations, gizmos, and bounds UI
        FragColor = baseColor;
    }
}

// --- Light Calculation Implementations ---

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 baseColor, vec3 specMap)
{
    vec3 lightDir = normalize(-light.direction);
    
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
    
    // Combine results using optimized parameters passed from main
    vec3 ambient  = light.ambient  * baseColor;
    vec3 diffuse  = light.diffuse  * diff * baseColor;
    vec3 specular = light.specular * spec * specMap;
    
    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor, vec3 specMap)
{
    vec3 lightDir = normalize(light.position - fragPos);
    
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
    
    // Attenuation calculation (Hardcoded 1.0 for constant to match your C++ struct)
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (1.0 + light.linear * distance + light.quadratic * (distance * distance));    
    
    // Combine results using your unified color and intensity system
    vec3 ambient  = 0.05 * light.color * light.intensity * baseColor;
    vec3 diffuse  = light.color * light.intensity * diff * baseColor;
    vec3 specular = light.color * light.intensity * spec * specMap;
    
    return (ambient + diffuse + specular) * attenuation;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor, vec3 specMap)
{
    vec3 lightDir = normalize(light.position - fragPos);
    
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
    
    // Attenuation calculation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    
    // Spotlight cone intensity evaluation
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    // Combine results
    vec3 ambient  = light.ambient  * baseColor;
    vec3 diffuse  = light.diffuse  * diff * baseColor;
    vec3 specular = light.specular * spec * specMap;
    
    return (ambient + (diffuse + specular) * intensity) * attenuation;
}
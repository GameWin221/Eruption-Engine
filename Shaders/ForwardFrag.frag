#version 450

#include "../EruptionEngine.ini"

layout(location = 0) in vec3 fPosition;
layout(location = 1) in vec3 fNormal;
layout(location = 2) in vec2 fTexcoord;
layout(location = 3) in mat3 fTBN;
layout(location = 6) in flat uint fMaterialIndex;

layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 3) uniform sampler2D albedoTextures[1024];
layout(set = 0, binding = 4) uniform sampler2D roughnessTextures[1024];
layout(set = 0, binding = 5) uniform sampler2D normalTextures[1024];
layout(set = 0, binding = 6) uniform sampler2D metalnessTextures[1024];

struct Material
{
	vec3 color;

    float metalnessVal;
    float roughnessVal;
    float normalMultiplier;

    uint albedoIndex;
    uint roughnessIndex;
    uint normalIndex;
    uint metalnessIndex;
};

layout (set = 0, std430, binding = 2) buffer Materials {
    Material materials[];
};

void main() 
{
    Material material = materials[fMaterialIndex];

    vec3 albedo   = texture(albedoTextures[material.albedoIndex], fTexcoord).rgb * material.color;
    vec3 position = fPosition.xyz;
    
    vec3 normalTg = 2.0 * texture(normalTextures[material.normalIndex], fTexcoord).xyz - 1.0;
    vec3 normal   = mix(fNormal, normalize(fTBN * normalize(normalTg)), material.normalMultiplier);

    float roughness = texture(roughnessTextures[material.roughnessIndex], fTexcoord).r * material.roughnessVal;
    float metalness = texture(metalnessTextures[material.metalnessIndex], fTexcoord).r * material.metalnessVal;

    FragColor = vec4(albedo, 1.0);
}
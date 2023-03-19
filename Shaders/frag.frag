#version 450

#include "../EruptionEngine.ini"

layout(location = 0) in vec3 fPosition;
layout(location = 1) in vec3 fNormal;
layout(location = 2) in vec2 fTexcoord;

layout(location = 0) out vec4 FragColor;

layout(set = 1, binding = 1) uniform sampler2D albedoTextures[MAX_TEXTURES];
//layout(set = 1, binding = 2) uniform sampler2D roughnessTextures[MAX_TEXTURES];
//layout(set = 1, binding = 3) uniform sampler2D metalnessTextures[MAX_TEXTURES];
//layout(set = 1, binding = 4) uniform sampler2D normalTextures[MAX_TEXTURES];

struct Material {
	vec3 color;
	float _padding0;

	float metalnessVal;
	float roughnessVal;
	float normalStrength;
	float _padding1;

	uint albedoId;
	uint roughnessId;
	uint metalnessId;
	uint normalId;
};

layout (set = 1, std430, binding = 2) buffer Materials {
	Material materials[];
};

layout(push_constant) uniform MaterialID {
	layout(offset = 4) 
	uint materialId;
};

void main()
{
    FragColor = vec4(texture(albedoTextures[materials[materialId].albedoId], fTexcoord).rgb, 1.0);
}
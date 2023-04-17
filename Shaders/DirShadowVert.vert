#version 450

#include "../EruptionEngine.ini"
#include "lights.glsl"
#include "camera.glsl"

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vTangent;
layout(location = 3) in vec2 vTexcoord;

layout(location = 0) out float fDistance;

layout (set = 0, std430, binding = 0) buffer ModelMatrices {
    mat4 modelMatrix[];
};

layout(std430, set = 0, binding = 1) buffer Lights
{
    PointLight pointLights[MAX_POINT_LIGHTS];
    SpotLight  spotLights[MAX_SPOT_LIGHTS];
    DirLight   dirLights[MAX_DIR_LIGHTS];

    uint activePointLights;
    uint activeSpotLights;
    uint activeDirLights;
    float _padding0;

    vec3 ambientLight;
    float _padding1;

    vec4 cascadeSplitDistances[SHADOW_CASCADES];
	vec4 cascadeFrustumSizeRatios[SHADOW_CASCADES];
};

layout(set = 1, binding = 0) uniform CameraBuffer {
	CameraBufferObject camera;
};

layout(push_constant) uniform PushConstant {
	uint modelMatrixID;
	uint lightID;
	uint cascadeID;
};

void main() 
{
    gl_Position = camera.cascadeMatrices[lightID][cascadeID] * modelMatrix[modelMatrixID] * vec4(vPosition, 1.0);
}
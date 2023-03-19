#version 450

#include "camera.glsl"

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vTangent;
layout(location = 3) in vec2 vTexcoord;

layout(location = 0) out vec3 fPosition;
layout(location = 1) out vec3 fNormal;
layout(location = 2) out vec2 fTexcoord;
//layout(location = 3) out mat3 fTBN;

layout(set = 0, binding = 0) uniform CameraBuffer {
	CameraBufferObject camera;
};

layout (set = 1, std430, binding = 0) buffer ModelMatrices {
    mat4 modelMatrix[];
};

layout(push_constant) uniform ModelMatrixID {
	uint modelMatrixID;
};

void main() 
{
	vec4 worldPos = modelMatrix[modelMatrixID] * vec4(vPosition, 1.0);

    gl_Position = camera.projView * worldPos;

	fPosition = worldPos.xyz;
	fNormal = vNormal;
	fTexcoord = vTexcoord;
}
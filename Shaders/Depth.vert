#version 450

#include "camera.glsl"

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexcoord;

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
	vec4 vWorldSpace = modelMatrix[modelMatrixID] * vec4(vPosition, 1.0);
    gl_Position = camera.projView * vWorldSpace;
}
#version 450

#include "camera.glsl"

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vTangent;
layout(location = 3) in vec2 vTexcoord;

layout(location = 0) out vec3 fPosition;
layout(location = 1) out vec3 fNormal;
layout(location = 2) out vec2 fTexcoord;
layout(location = 3) out mat3 fTBN;
layout(location = 6) out flat uint fMaterialIndex;

struct Index {
	uint matrixIndex;
	uint materialIndex;
};

layout(set = 1, binding = 0) uniform CameraBuffer {
	CameraBufferObject camera;
};

layout (set = 0, std430, binding = 0) buffer ObjectIndices {
	Index indices[];
};

layout (set = 0, std430, binding = 1) buffer ModelMatrices {
    mat4 model[];
};

void main() 
{
	uint matrixIndex = indices[gl_InstanceIndex].matrixIndex;

	vec4 worldPos = model[matrixIndex] * vec4(vPosition, 1.0);

    gl_Position = camera.projView * worldPos;

	fPosition = worldPos.xyz;
	fNormal = vNormal;
	fTexcoord = vTexcoord;

	vec3 tangent = normalize(vec3(model[matrixIndex] * vec4(vTangent, 0.0)));

    // Reorthogonalize vertex tangents relatively to normals
    tangent = normalize(tangent - dot(tangent, fNormal) * fNormal); 

    // Calculate bitangents with cross product
    vec3 bitangent = normalize(cross(tangent, fNormal));

    fTBN = mat3(tangent, bitangent, fNormal);
}
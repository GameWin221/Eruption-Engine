#version 450

#include "camera.glsl"

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vTangent;
layout(location = 3) in vec2 vTexcoord;

layout(location = 0) out vec4 fPosition;
layout(location = 1) out vec3 fNormal;
layout(location = 2) out vec2 fTexcoord;
layout(location = 3) out mat3 fTBN;
layout(location = 6) noperspective out vec2 fUV;

layout(set = 0, binding = 0) uniform CameraBuffer {
	CameraBufferObject camera;
};

layout (set = 1, std430, binding = 0) buffer ModelMatrices {
    mat4 modelMatrix[];
};

layout(push_constant) uniform ModelMatrixID {
	uint modelMatrixID;
};

float LinearDepth(float d)
{
    return camera.zNear * camera.zFar / (camera.zFar + d * (camera.zNear - camera.zFar));
}
void main() 
{
	vec4 vWorldSpace = modelMatrix[modelMatrixID] * vec4(vPosition, 1.0);

    gl_Position = camera.projView * vWorldSpace;

    // Screen UV
    fUV = (gl_Position.xy / gl_Position.w) / 2.0 + 0.5;
    
    // Transform vertex positions to world space
    fPosition = vec4(vWorldSpace.xyz, LinearDepth(gl_Position.z / gl_Position.w));

    // Transform vertex normals and tangents to world space
    fNormal  = normalize(vec3(modelMatrix[modelMatrixID] * vec4(vNormal , 0.0)));
    vec3 tangent = normalize(vec3(/*camera.view * */modelMatrix[modelMatrixID] * vec4(vTangent, 0.0)));

    // Reorthogonalize vertex tangents relatively to normals
    tangent = normalize(tangent - dot(tangent, fNormal) * fNormal); 

    // Calculate bitangents with cross product
    vec3 bitangent = normalize(cross(tangent, fNormal));

    fTBN = mat3(tangent, bitangent, fNormal);
    
    fTexcoord = vTexcoord;
}
#version 450

#include "../EruptionEngine.ini"

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vTangent;
layout(location = 3) in vec2 vTexcoord;

layout(location = 0) out vec4 fPosition;
layout(location = 1) out vec3 fNormal;
layout(location = 2) out vec2 fTexcoord;
layout(location = 3) out mat3 fTBN;

layout(set = 0, binding = 0) uniform CameraBufferObject
{
    mat4 view;
	mat4 invView;
	mat4 proj;
	mat4 projView;

	vec3 position;

	int debugMode;

	vec4 cascadeSplitDistances[SHADOW_CASCADES];
	vec4 cascadeFrustumSizeRatios[SHADOW_CASCADES];

	uvec4 clusterTileCount;
	uvec4 clusterTileSizes;

	float clusterScale;
	float clusterBias;

    float zNear;
	float zFar;
} camera;

layout(push_constant) uniform PerObjectData
{
	mat4 model;
} object;

float LinearDepth(float d)
{
    return camera.zNear * camera.zFar / (camera.zFar + d * (camera.zNear - camera.zFar));
}

void main() 
{
    gl_Position = camera.projView * object.model * vec4(vPos, 1.0);

    // Transform vertex positions to world space
    fPosition = vec4(vec3(object.model * vec4(vPos, 1.0)), LinearDepth(gl_Position.z / gl_Position.w));

    // Transform vertex normals and tangents to world space
    fNormal  = normalize(vec3(object.model * vec4(vNormal , 0.0)));
    vec3 tangent = normalize(vec3(camera.view * object.model * vec4(vTangent, 0.0)));

    // Reorthogonalize vertex tangents relatively to normals
    tangent = normalize(tangent - dot(tangent, fNormal) * fNormal); 

    // Calculate bitangents with cross product
    vec3 bitangent = normalize(cross(tangent, fNormal));

    fTBN = mat3(tangent, bitangent, fNormal);
    
    fTexcoord = vTexcoord;
}
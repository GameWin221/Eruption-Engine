#version 450

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vTangent;
layout(location = 3) in vec2 vTexcoord;

layout(set = 0, binding = 0) uniform CameraMatricesBufferObject
{
    mat4 view;
    mat4 proj;
} camera;

layout(push_constant) uniform PerObjectData
{
	mat4 model;
} object;

void main() 
{
    gl_Position = camera.proj * camera.view * object.model * vec4(vPos, 1.0);
}
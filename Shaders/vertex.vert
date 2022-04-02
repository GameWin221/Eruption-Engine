#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexcoord;

layout(location = 0) out vec3 fNormal;
layout(location = 1) out vec2 fTexcoord;

void main() 
{
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(vPos, 1.0);

    fNormal = mat3(transpose(inverse(ubo.model))) * vNormal;
    fTexcoord = vTexcoord;
}
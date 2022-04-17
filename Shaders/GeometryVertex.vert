#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;

    vec3 color;
    float specularity;
} ubo;

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexcoord;

layout(location = 0) out vec3 fNormal;
layout(location = 1) out vec2 fTexcoord;
layout(location = 2) out vec3 fPosition;
layout(location = 3) out vec3 fColor;

void main() 
{
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(vPos, 1.0);

    fPosition = vec3(ubo.model * vec4(vPos, 1.0));
    fNormal   = mat3(transpose(inverse(ubo.model))) * vNormal;
    fTexcoord = vTexcoord;

    fColor = ubo.color;
}
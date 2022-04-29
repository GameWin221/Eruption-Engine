#version 450

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vTangent;
layout(location = 3) in vec2 vTexcoord;

layout(location = 0) out vec3 fNormal;
layout(location = 1) out vec2 fTexcoord;
layout(location = 2) out vec3 fPosition;
layout(location = 3) out vec3 fTangent;
layout(location = 4) out vec3 fBitangent;


layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() 
{
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(vPos, 1.0);

    // Transform vertex positions to model space
    fPosition = vec3(ubo.model * vec4(vPos, 1.0));

    // Transform vertex normals and tangents to model space
    fNormal  = normalize(vec3(ubo.model * vec4(vNormal , 0.0)));
    fTangent = normalize(vec3(ubo.model * vec4(vTangent, 0.0)));

    // Reorthogonalize vertex tangents relatively to normals
    fTangent = normalize(fTangent - dot(fTangent, fNormal) * fNormal); 

    // Calculate bitangents with cross product
    fBitangent = normalize(cross(fTangent, fNormal));

    fTexcoord = vTexcoord;
}
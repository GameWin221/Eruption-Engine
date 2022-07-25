#version 450

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vTangent;
layout(location = 3) in vec2 vTexcoord;

layout(push_constant) uniform DepthInfo
{
	mat4 model;
	mat4 viewProj;
} info;

void main() 
{
    gl_Position = info.viewProj * info.model * vec4(vPos, 1.0);
}
#version 450

#include "../EruptionEngine.ini"

layout(location = 0) in vec3 fPosition;
layout(location = 1) in vec3 fNormal;
layout(location = 2) in vec2 fTexcoord;

layout(location = 0) out vec4 FragColor;

void main() 
{
    FragColor = vec4(1.0, 0.0, 1.0, 1.0);
}
#version 450

#include "../EruptionEngine.ini"

layout(location = 0) in vec3 fPosition;
layout(location = 1) in vec3 fNormal;
layout(location = 2) in vec2 fTexcoord;

layout(location = 0) out vec4 FragColor;

void main() 
{
    vec3 color = vec3(
        (10 % int(fPosition.x*10.0))/10.0,
        (10 % int(fPosition.y*10.0))/10.0,
        (10 % int(fPosition.z*10.0))/10.0
    );

    FragColor = vec4(color, 1.0);
}
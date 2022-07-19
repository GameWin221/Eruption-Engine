#version 450

#include "../EruptionEngine.ini"

layout(location = 0) in vec3 fPosition;
layout(location = 1) in vec3 lPos;
layout(location = 2) in float lFarPlane;

layout(location = 0) out vec4 Depth;

void main() 
{
	float lightDistance = length(fPosition - lPos);
    
    lightDistance = lightDistance / lFarPlane;

    Depth.r = lightDistance;
}
#version 450

layout(location = 0) in float fDistance;
layout(location = 0) out float Shadowmap;

void main()
{
    Shadowmap = fDistance;
}
#version 450

layout(binding = 0) uniform sampler2D AliasedImage;

layout(location = 0) in vec2 fTexcoord;
layout(location = 0) out vec4 AntialiasedResult;

void main() 
{
	AntialiasedResult = vec4(0.0, 0.0, 0.0, 1.0);
}
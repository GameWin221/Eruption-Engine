#version 450

layout(location = 0) in vec2 fTexcoord;
layout(location = 0) out vec4 Swapchain;

layout(binding = 0) uniform sampler2D LDRImage;

void main() 
{
    Swapchain = vec4(texture(LDRImage, fTexcoord).rgb, 1.0);
}
#version 450

layout(location = 0) in vec3 fNormal;
layout(location = 1) in vec2 fTexcoord;

layout(location = 0) out vec4 FragColor;

layout(binding = 1) uniform sampler2D mTexture;

void main() 
{
    FragColor = texture(mTexture, fTexcoord);
}
#version 450

layout(location = 0) in vec3 fNormal;
layout(location = 1) in vec2 fTexcoord;
layout(location = 2) in vec3 fPosition;

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gPosition;
layout(location = 2) out vec4 gNormal;

layout(binding = 1) uniform sampler2D mTexture;

const float specularity = 0.5;

void main() 
{
    gColor = texture(mTexture, fTexcoord);

    gPosition.xyz = fPosition;
    gPosition.a = specularity;

    gNormal.xyz = normalize(fNormal);
}
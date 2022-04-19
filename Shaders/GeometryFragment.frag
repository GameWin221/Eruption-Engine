#version 450

layout(location = 0) in vec3 fNormal;
layout(location = 1) in vec2 fTexcoord;
layout(location = 2) in vec3 fPosition;

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gPosition;
layout(location = 2) out vec4 gNormal;

layout(set = 1, binding = 1) uniform MatBufferObject {
    vec3 color;
    float shininess;
} mbo;

layout(set = 1, binding = 2) uniform sampler2D albedoTexture;
layout(set = 1, binding = 3) uniform sampler2D specularTexture;

void main() 
{
    gColor = vec4(texture(albedoTexture, fTexcoord).rgb * mbo.color, texture(specularTexture, fTexcoord).r);

    gPosition = vec4(fPosition, mbo.shininess);

    gNormal = vec4(normalize(fNormal), 1.0);
}
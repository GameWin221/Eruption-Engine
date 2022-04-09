#version 450

#include "../EruptionEngine.ini"

layout(location = 0) in vec2 fTexcoord;

layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform sampler2D gColor;
layout(binding = 1) uniform sampler2D gPosition;
layout(binding = 2) uniform sampler2D gNormal;

struct PointLight
{
    vec3 position;
    vec3 color;
};

layout(binding = 3) uniform UniformBufferObject {
    PointLight lights[MAX_LIGHTS];
} lightsBuffer;

vec3 CalculateLight(int lightIndex, vec3 position, vec3 normal)
{
    vec3 lightDir = normalize(lightsBuffer.lights[lightIndex].position - position);  
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = lightsBuffer.lights[lightIndex].color * diff;
    
    return diffuse;
}

void main() 
{
    vec3 color = texture(gColor, fTexcoord).rgb;
    vec3 normal = texture(gNormal, fTexcoord).rgb;
    vec3 position = texture(gPosition, fTexcoord).rgb;
    
    //float specularity = texture(gPosition, fTexcoord).a;
    
    vec3 diffuse = vec3(0);

    for(int i = 0; i < MAX_LIGHTS; i++)
        diffuse += CalculateLight(i, position, normal);

    vec3 result = color * diffuse;

    FragColor = vec4(result, 1.0);
}
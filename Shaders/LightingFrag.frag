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
    float radius;
};

layout(binding = 3) uniform UBO 
{
    PointLight lights[MAX_LIGHTS];
} lightsBuffer;

vec3 CalculateLight(int lightIndex, vec3 position, vec3 normal, float dist)
{
    vec3  lPos = lightsBuffer.lights[lightIndex].position;
    vec3  lCol = lightsBuffer.lights[lightIndex].color; 
    float lRad = lightsBuffer.lights[lightIndex].radius; 

    float attenuation = max(1.0 - dist*dist/(lRad*lRad), 0.0); 
    attenuation *= attenuation;

    vec3 lightDir = normalize(lPos - position); 
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = lCol * diff * attenuation;
    
    return diffuse;
}

void main() 
{
    vec3 color = texture(gColor, fTexcoord).rgb;
    vec3 normal = texture(gNormal, fTexcoord).rgb;
    vec3 position = texture(gPosition, fTexcoord).rgb;
    
    float specularity = texture(gColor, fTexcoord).a;
    
    vec3 diffuse = vec3(0);

    for(int i = 0; i < MAX_LIGHTS; i++)
    {
        if(lightsBuffer.lights[i].color != vec3(0))
        {
            vec3 lfDiff = position - lightsBuffer.lights[i].position;
            float lfDist = length(lfDiff);

            if(lfDist < lightsBuffer.lights[i].radius)
                diffuse += CalculateLight(i, position, normal, lfDist);
        }
    }
        

    vec3 result = color * diffuse;

    FragColor = vec4(result, 1.0);
}
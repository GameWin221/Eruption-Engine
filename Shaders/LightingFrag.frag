#version 450

#include "../EruptionEngine.ini"

layout(location = 0) in vec2 fTexcoord;

layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform sampler2D gColor;
layout(binding = 1) uniform sampler2D gPosition;
layout(binding = 2) uniform sampler2D gNormal;

const vec3 ambientLight = vec3(0.03);
const float baseSpecularity = 32.0f;

struct PointLight
{
    vec3 position;
    vec3 color;
    float radius;
};

layout(binding = 3) uniform UBO 
{
    PointLight lights[MAX_LIGHTS];
    vec3 viewPos;
} lightsBuffer;


float whenNotEqual(float x, float y) {
    return abs(sign(x - y));
}

vec3 CalculateLight(int lightIndex, vec3 color, float specularity, vec3 position, vec3 normal, float dist)
{
    vec3  lPos = lightsBuffer.lights[lightIndex].position;
    vec3  lCol = lightsBuffer.lights[lightIndex].color; 
    float lRad = lightsBuffer.lights[lightIndex].radius; 

    // Attenuation Calculation
    float attenuation = max(1.0 - dist*dist/(lRad*lRad), 0.0); 
    attenuation *= attenuation;

    // Diffuse Lighting
    vec3 lightDir = normalize(lPos - position); 
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = color * lCol * diff;
    
    // Specular Lighting
    vec3 viewDir = normalize(lightsBuffer.viewPos - position);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), baseSpecularity);
    spec *= whenNotEqual(diff, 0.0);
    vec3 specular = lCol * spec * specularity;

    vec3 result = (diffuse + specular) * attenuation;
    return result;
}

// 0 - 5
const int debugView = 0;

void main() 
{
    vec3 color = texture(gColor, fTexcoord).rgb;
    vec3 normal = texture(gNormal, fTexcoord).rgb;
    vec3 position = texture(gPosition, fTexcoord).rgb;
    float specularity = texture(gColor, fTexcoord).a;
    
    vec3 ambient = color * ambientLight;
    vec3 lighting = vec3(0);

    for(int i = 0; i < MAX_LIGHTS; i++)
    {
        if(lightsBuffer.lights[i].color != vec3(0))
        {
            float dist = length(position - lightsBuffer.lights[i].position);

            if(dist < lightsBuffer.lights[i].radius)
                lighting += CalculateLight(i, color, specularity, position, normal, dist);
        }
    }
    
    vec3 result = vec3(0);

    switch(debugView)
    {
        case 0:
            result = ambient + lighting;
            break;
        case 1:
            result = color;
            break;
        case 2:
            result = normal;
            break;
        case 3:
            result = position;
            break;
        case 4:
            result = vec3(specularity);
            break;
        case 5:
            result = lighting;
            break;
    }
    

    FragColor = vec4(result, 1.0);
}
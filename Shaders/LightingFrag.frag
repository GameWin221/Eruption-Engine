#version 450

#include "../EruptionEngine.ini"

#define PI 3.14159265359

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

    uint activePointLights;

    vec3 ambient;

} lightsBuffer;

layout(push_constant) uniform LightsCameraInfo
{
	vec3 viewPos;
    int debugMode;
} camera;



float WhenNotEqual(float x, float y) {
    return abs(sign(x - y));
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

vec3 CalculateLight(PointLight light, vec3 albedo, float roughness, float metalness, vec3 F0, vec3 viewDir, vec3 position, vec3 normal, float dist)
{
    float attenuation = max(1.0 - dist*dist/(light.radius*light.radius), 0.0); 
    attenuation *= attenuation;

    vec3 lightDir = normalize(light.position - position); 
    vec3 H = normalize(lightDir + viewDir);

    float cosTheta = max(dot(normal, lightDir), 0.0);
    vec3  radiance  = light.color * attenuation * cosTheta;

    float NDF = DistributionGGX(normal, H, roughness);       
    float G   = GeometrySmith(normal, viewDir, lightDir, roughness);    
    vec3  F   = FresnelSchlick(max(dot(H, viewDir), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metalness;	

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0)  + 0.0001;
    vec3 specular     = numerator / denominator;  

    float NdotL = max(dot(normal, lightDir), 0.0);        

    return (kD * albedo / PI + specular) * radiance * NdotL;
}

void main() 
{
    vec4 gColorSample    = texture(gColor   , fTexcoord);
    vec4 gNormalSample   = texture(gNormal  , fTexcoord);
    vec4 gPositionSample = texture(gPosition, fTexcoord);

    vec3  albedo    = gColorSample.rgb;
    vec3  normal    = gNormalSample.rgb;
    vec3  position  = gPositionSample.rgb;

    float roughness = gColorSample.a;
    float metalness = gPositionSample.a;

    float depth = length(camera.viewPos-position);

    vec3 ambient = albedo * lightsBuffer.ambient;
    vec3 lighting = vec3(0.0f);

    vec3 F0 = mix(vec3(0.04), albedo, metalness);

    vec3 viewDir = normalize(camera.viewPos - position);

    for(int i = 0; i < lightsBuffer.activePointLights; i++)
    {
        float dist = length(position - lightsBuffer.lights[i].position);

        if(dist < lightsBuffer.lights[i].radius)
            lighting += CalculateLight(lightsBuffer.lights[i], albedo, roughness, metalness, F0, viewDir, position, normal, dist);
    }
    
    vec3 result = vec3(0.0f);
    
    switch(camera.debugMode)
    {
        case 0:
            result = ambient + lighting;
            break;
        case 1:
            result = albedo;
            break;
        case 2:
            result = normal;
            break;
        case 3:
            result = position;
            break;
        case 4:
            result = vec3(roughness);
            break;
        case 5:
            result = vec3(metalness);
            break;
        case 6:
            result = vec3(depth);
            break;
    }
    
    FragColor = vec4(result, 1.0);
}
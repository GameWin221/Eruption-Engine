#version 450

#include "../EruptionEngine.ini"

#define PI 3.14159265359

layout(location = 0) in vec2 fTexcoord;
layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform sampler2D gColor;
layout(binding = 1) uniform sampler2D gPosition;
layout(binding = 2) uniform sampler2D gNormal;
layout(binding = 3) uniform texture2D dirShadowmaps[MAX_DIR_LIGHT_SHADOWS];
layout(binding = 4) uniform sampler shadowmapSampler;// TODO:  Bind the sampler 

struct PointLight
{
    vec3 position;
    float radius;
    vec3 color;
};
struct SpotLight
{
    vec3 position;
    float innerCutoff;
    vec3 direction;
    float outerCutoff;
    vec3 color;
    float range;
};
struct DirLight
{
    vec3 direction;
    int shadowmapIndex;
    vec3 color;
    mat4 projView;
};

layout(binding = 5) uniform UBO 
{
    PointLight pLights[MAX_POINT_LIGHTS];
    SpotLight  sLights[MAX_SPOT_LIGHTS];
    DirLight   dLights[MAX_DIR_LIGHTS];

    uint activePointLights;
    uint activeSpotLights;
    uint activeDirLights;

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

float CalculateShadow(vec4 fPosLightSpace, int shadowmapIndex)
{
    vec3 projCoords = fPosLightSpace.xyz / fPosLightSpace.w;

    //projCoords = projCoords * 0.5 + 0.5; 
    float closestDepth = texture(sampler2D(dirShadowmaps[shadowmapIndex], shadowmapSampler), projCoords.xy).r;   
    float currentDepth = projCoords.z;  
    float shadow = currentDepth > closestDepth  ? 1.0 : 0.0;

    return shadow;
}

vec3 CalculatePointLight(PointLight light, vec3 albedo, float roughness, float metalness, vec3 F0, vec3 viewDir, vec3 position, vec3 normal, float dist)
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

    vec3 kD = vec3(1.0) - F;
    kD *= 1.0 - metalness;	

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0)  + 0.0001;
    vec3 specular     = numerator / denominator;  

    float NdotL = max(dot(normal, lightDir), 0.0);        

    return (kD * albedo / PI + specular) * radiance * NdotL;
}
vec3 CalculateDirLight(DirLight light, vec3 albedo, float roughness, float metalness, vec3 F0, vec3 viewDir, vec3 position, vec3 normal)
{
    vec3 lightDir = normalize(light.direction); 
    vec3 H = normalize(lightDir + viewDir);

    float cosTheta = max(dot(normal, lightDir), 0.0);
    vec3  radiance  = light.color * cosTheta;

    float NDF = DistributionGGX(normal, H, roughness);       
    float G   = GeometrySmith(normal, viewDir, lightDir, roughness);    
    vec3  F   = FresnelSchlick(max(dot(H, viewDir), 0.0), F0);

    vec3 kD = vec3(1.0) - F;
    kD *= 1.0 - metalness;	

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0)  + 0.0001;
    vec3 specular     = numerator / denominator;  

    float NdotL = max(dot(normal, lightDir), 0.0);        

    return (kD * albedo / PI + specular) * radiance * NdotL;
}
vec3 CalculateSpotLight(SpotLight light, vec3 albedo, float roughness, float metalness, vec3 F0, vec3 viewDir, vec3 position, vec3 normal, float theta)
{
    float dist = length(position - light.position);

    float attenuation = max(1.0 - dist*dist/(light.range*light.range), 0.0); 
    attenuation *= attenuation;

    vec3 lightDir = normalize(light.position - position); 
    vec3 H = normalize(lightDir + viewDir);

    float cosTheta = max(dot(normal, lightDir), 0.0);
    vec3  radiance  = light.color * attenuation * cosTheta;

    float NDF = DistributionGGX(normal, H, roughness);       
    float G   = GeometrySmith(normal, viewDir, lightDir, roughness);    
    vec3  F   = FresnelSchlick(max(dot(H, viewDir), 0.0), F0);

    vec3 kD = vec3(1.0) - F;
    kD *= 1.0 - metalness;	

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0)  + 0.0001;
    vec3 specular     = numerator / denominator;  

    float NdotL = max(dot(normal, lightDir), 0.0);        

    //float intensity = theta*theta;// * (1.0/light.directionOuterCutoff.w);

    return (kD * albedo / PI + specular) * radiance * NdotL;
}

void main() 
{
    vec4 gColorSample    = texture(gColor   , fTexcoord);
    vec4 gNormalSample   = texture(gNormal  , fTexcoord);
    vec4 gPositionSample = texture(gPosition, fTexcoord);

    vec3 albedo    = gColorSample.rgb;
    vec3 normal    = gNormalSample.rgb;
    vec3 position  = gPositionSample.rgb;

    float roughness = gColorSample.a;
    float metalness = gPositionSample.a;

    float depth = length(camera.viewPos-position);

    vec3 ambient = albedo * lightsBuffer.ambient;
    vec3 lighting = vec3(0.0);
    float shadow = 0.0;

    vec3 F0 = mix(vec3(0.04), albedo, metalness);

    vec3 viewDir = normalize(camera.viewPos - position);

    for(int i = 0; i < lightsBuffer.activePointLights; i++)
    {
        float dist = length(position - lightsBuffer.pLights[i].position);

        if(dist < lightsBuffer.pLights[i].radius)
            lighting += CalculatePointLight(lightsBuffer.pLights[i], albedo, roughness, metalness, F0, viewDir, position, normal, dist);
    }
    for(int i = 0; i < lightsBuffer.activeSpotLights; i++)
    {
        vec3 lightToSurfaceDir = normalize(position - lightsBuffer.sLights[i].position);

        float innerCutoff = 1.0-lightsBuffer.sLights[i].innerCutoff;
        float outerCutoff = 1.0-lightsBuffer.sLights[i].outerCutoff;

        float theta     = dot(normalize(lightsBuffer.sLights[i].direction), lightToSurfaceDir);
        float epsilon   = innerCutoff - outerCutoff;
        float intensity = clamp((theta - outerCutoff) / epsilon, 0.0, 1.0);    

        if(theta > outerCutoff)
            lighting += CalculateSpotLight(lightsBuffer.sLights[i], albedo, roughness, metalness, F0, viewDir, position, normal, theta) * intensity;
    }
    for(int i = 0; i < lightsBuffer.activeDirLights; i++)
    {
        if(lightsBuffer.dLights[i].shadowmapIndex != -1)
        {
            vec4 fPosLightSpace = lightsBuffer.dLights[i].projView * vec4(position, 1.0);
            shadow += CalculateShadow(fPosLightSpace, lightsBuffer.dLights[i].shadowmapIndex);
        }

        lighting += CalculateDirLight(lightsBuffer.dLights[i], albedo, roughness, metalness, F0, viewDir, position, normal);
    }

    vec3 result = vec3(0.0f);
    
    switch(camera.debugMode)
    {
        case 0:
            result = lighting * (1.0 - shadow) + ambient;
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
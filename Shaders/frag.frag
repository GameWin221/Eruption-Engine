#version 450

#include "../EruptionEngine.ini"
#include "camera.glsl"

layout(location = 0) in vec4 fPosition;
layout(location = 1) in vec3 fNormal;
layout(location = 2) in vec2 fTexcoord;
layout(location = 3) in mat3 fTBN;
layout(location = 6) noperspective in vec2 fUV;

layout(location = 0) out vec4 FragColor;

layout(set = 1, binding = 1) uniform sampler2D textures[MAX_TEXTURES];
//layout(set = 1, binding = 2) uniform sampler2D roughnessTextures[MAX_TEXTURES];
//layout(set = 1, binding = 3) uniform sampler2D metalnessTextures[MAX_TEXTURES];
//layout(set = 1, binding = 4) uniform sampler2D normalTextures[MAX_TEXTURES];

layout(set = 0, binding = 0) uniform CameraBuffer {
	CameraBufferObject camera;
};

struct Material {
	vec3 color;
	float _padding0;

	float metalnessVal;
	float roughnessVal;
	float normalStrength;
	float _padding1;

	uint albedoId;
	uint roughnessId;
	uint metalnessId;
	uint normalId;
};

layout (set = 1, std430, binding = 2) buffer Materials {
	Material materials[];
};

layout(push_constant) uniform MaterialID {
	layout(offset = 4) 
	uint materialId;
};

struct PointLight
{
    vec3 position;
    float radius;

    vec3 color;
	int shadowmapIndex;

	float shadowSoftness;
    int pcfSampleRate; 
    float bias;
    float _padding0;
};
struct SpotLight
{
    vec3 position;
    float innerCutoff;

    vec3 direction;
    float outerCutoff;

    vec3 color;
    float range;

    //mat4 projView;

    int shadowmapIndex;
    float shadowSoftness;
    int pcfSampleRate; 
    float bias;
};
struct DirLight
{
    vec3 direction;
    int shadowmapIndex;

    vec3 color;
    float shadowSoftness;

    int pcfSampleRate;
    float bias;
    float _padding0;
    float _padding1;
};

layout(std430, set = 1, binding = 3) buffer Lights
{
    PointLight pointLights[MAX_POINT_LIGHTS];
    SpotLight  spotLights[MAX_SPOT_LIGHTS];
    DirLight   dirLights[MAX_DIR_LIGHTS];

    uint activePointLights;
    uint activeSpotLights;
    uint activeDirLights;
    float _padding0;

    vec3 ambientLight;
    float _padding1;
};

#define PI 3.14159265359

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
vec3 PBRLighting(vec3 lightDir, vec3 lightColor, vec3 albedo, float roughness, float metalness, vec3 normal, vec3 position, vec3 viewDir, vec3 F0)
{
    vec3 H = normalize(lightDir + viewDir);

    float cosTheta = max(dot(normal, lightDir), 0.0);
    vec3  radiance  = lightColor * cosTheta;

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

vec3 NormalMapping(uint textureId, float multiplier)
{
    vec3 normalMap = texture(textures[textureId], fTexcoord).xyz;
    normalMap = normalize(2.0 * normalMap - 1.0);
    
    vec3 result = normalize(fTBN * normalMap);
    result = mix(fNormal, result, multiplier);

    return result;
}
void main()
{
	Material material = materials[materialId];

    vec3  albedo    = texture(textures[material.albedoId], fTexcoord).rgb * material.color;
    vec3  normal    = NormalMapping(material.normalId, material.normalStrength);
    vec3  position  = fPosition.xyz;
    float roughness = texture(textures[material.roughnessId], fTexcoord).r * material.roughnessVal;
    float metalness = texture(textures[material.metalnessId], fTexcoord).r * material.metalnessVal;

    vec3 ambient = albedo * ambientLight;
    vec3 lighting = vec3(0.0);

    float linearDepth = fPosition.w;

    vec3 F0 = mix(vec3(0.04), albedo, metalness);

    vec3 viewDir   = normalize(camera.position - position);
    float viewDist = length(camera.position - position);

    for(uint i = 0; i < activePointLights; i++)
    {
        PointLight light = pointLights[i];

        float lightDist = length(position - light.position);
        vec3 lightDir = normalize(light.position - position); 

        float attenuation = max(1.0 - lightDist*lightDist/(light.radius*light.radius), 0.0); 
        attenuation *= attenuation;

        lighting += PBRLighting(lightDir, light.color, albedo, roughness, metalness, normal, position, viewDir, F0) * attenuation;
    }
    for(uint i = 0; i < activeSpotLights; i++)
    {
        SpotLight light = spotLights[i];

        vec3 lightDir = normalize(light.direction); 

        vec3 lightToSurfaceDir = normalize(position - light.position);
        
        float innerCutoff = light.innerCutoff / 2.0 * PI;
        float outerCutoff = light.outerCutoff / 2.0 * PI;
        
        float cosine = dot(light.direction, lightToSurfaceDir) / length(light.direction) * length(lightToSurfaceDir);
	    float angleToSurface = acos(cosine);

        float angleDiff = min(angleToSurface, outerCutoff);
        float intensity = pow(min(1.0 - (angleDiff-innerCutoff) / (outerCutoff-innerCutoff), 1.0), 2.0);

        lighting += PBRLighting(lightDir, light.color, albedo, roughness, metalness, normal, position, viewDir, F0) * intensity;
    }
    for(uint i = 0; i < activeDirLights; i++)
    {
        DirLight light = dirLights[i];

        vec3 lightDir = normalize(light.direction); 

        lighting += PBRLighting(lightDir, light.color, albedo, roughness, metalness, normal, position, viewDir, F0);
    }

    vec3 result = vec3(0.0);

    switch(camera.debugMode)
    {
        case 0:
            result = lighting + ambient;
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
            result = fTBN[0];
            //result = ambient + vec3(float(pointLightCount) / (lightsBuffer.activePointLights+1), float(spotLightCount) / (lightsBuffer.activeSpotLights+1), 0);
            break;
        case 7:
            result = fTBN[1];
            //const vec3 depthSplitColors[8] = vec3[](
            //    vec3(0, 0, 0), vec3( 0, 0, 1), vec3(0, 1, 0), vec3(0, 1, 1),
            //    vec3(1, 0, 0), vec3( 1, 0, 1), vec3(1, 1, 0), vec3(1, 1, 1)
            //);

            //result = lighting + ambient + depthSplitColors[uint(zTile - (8.0 * floor(zTile/8.0)))] * 0.18;
            break;
        case 8:
            //const vec3 cascadeColors[4] = vec3[](
            //    vec3(1, 0, 0), vec3( 0, 1, 0), vec3(0, 0, 1), vec3(0, 1, 1)
            //);
            result = vec3(fTexcoord, 0.0);
            //result = lighting + ambient + cascadeColors[uint(cascade - (4.0 * floor(cascade/4.0)))] * 0.18;
            break;
    }

    FragColor = vec4(result, 1.0);
}
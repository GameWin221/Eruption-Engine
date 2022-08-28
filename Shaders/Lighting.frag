#version 450

#include "../EruptionEngine.ini"

#define PI 3.14159265359

layout(location = 0) in vec2 fTexcoord;
layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform sampler2D gColor;
layout(binding = 1) uniform sampler2D gPosition;
layout(binding = 2) uniform sampler2D gNormal;
layout(binding = 3) uniform samplerCubeArray pointShadowmaps;
layout(binding = 4) uniform sampler2DArray spotShadowmaps;
layout(binding = 5) uniform sampler2DArray dirShadowmaps;

struct PointLight
{
    vec3 position;
    float radius;
    vec3 color;
	int shadowmapIndex;
	float shadowSoftness;
    int pcfSampleRate; 
    float bias;
};
struct SpotLight
{
    vec3 position;
    float innerCutoff;
    vec3 direction;
    float outerCutoff;
    vec3 color;
    float range;
    mat4 projView;
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
    
    mat4 projView[SHADOW_CASCADES];
};

layout(binding = 6) uniform UBO 
{
    PointLight pLights[MAX_POINT_LIGHTS];
    SpotLight  sLights[MAX_SPOT_LIGHTS];
    DirLight   dLights[MAX_DIR_LIGHTS];

    uint activePointLights;
    uint activeSpotLights;
    uint activeDirLights;
    float dummy0;

    vec3 ambient;
    float dummy1;

    vec4 cascadeSplitDistances[SHADOW_CASCADES];

    vec4 cascadeRatios[SHADOW_CASCADES];

    vec3 viewPos;
    int debugMode;

    mat4 cameraView;

} lightsBuffer;

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

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0
);

vec3 pcfSampleOffsets[20] = vec3[](
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);   

float CalculateShadow(vec4 fPosLightSpace, sampler2DArray shadowmaps, int shadowmapIndex, int sampleCount, float bias, float softness)
{
    vec3 projCoords = fPosLightSpace.xyz / fPosLightSpace.w;

    if(projCoords.z > 1.0)
        return 0.0;

    float shadow = 0.0;

#if SOFT_SHADOWS
    vec2 texelSize = vec2(1.0) / DIR_SHADOWMAP_RES * softness;
    for(int x = -sampleCount; x <= sampleCount; ++x)
    {
        for(int y = -sampleCount; y <= sampleCount; ++y)
        {
            float pcfDepth = texture(shadowmaps, vec3(projCoords.xy + vec2(float(x)/sampleCount, float(y)/sampleCount) * texelSize, shadowmapIndex)).r; 
            shadow += projCoords.z - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }

    float divider = sampleCount * 2.0 + 1.0;

    shadow /= divider * divider;
#else
    float closestDepth = texture(shadowmaps, vec3(projCoords.xy, shadowmapIndex)).r;    
    shadow = projCoords.z - bias > closestDepth ? 1.0 : 0.0;
#endif

    return shadow;
}

float CalculateOmniShadows(vec3 fragToLightDiff, float viewToFragDist, float farPlane, int shadowmapIndex, int sampleCount, float bias, float softness)
{
    float shadow = 0.0;
    float currentDepth = length(fragToLightDiff);

    #if SOFT_SHADOWS
        for(int i = 0; i < 20; ++i)
        {
            for(int j = 1; j <= sampleCount; j++)
            {
                float interpolator = float(j) / sampleCount;
                float closestDepth = texture(pointShadowmaps, vec4(fragToLightDiff + pcfSampleOffsets[i] * softness * interpolator * 0.01, shadowmapIndex)).r * farPlane;

                shadow += currentDepth - bias > closestDepth ? 1.0 : 0.0;
            }
        }
        shadow /= 20.0 * sampleCount;  
    #else
        float closestDepth = texture(pointShadowmaps, vec4(fragToLightDiff, shadowmapIndex)).r * farPlane;

        shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    #endif

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
vec3 CalculateSpotLight(SpotLight light, vec3 albedo, float roughness, float metalness, vec3 F0, vec3 viewDir, vec3 position, vec3 normal)
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

    return (kD * albedo / PI + specular) * radiance * NdotL;
}

float RadiansBetweenDirs(vec3 a, vec3 b)
{
	float cosine = dot(a, b) / length(a) * length(b);
	float angleRadians = acos(cosine);
	return angleRadians;
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

    float depth = length(lightsBuffer.viewPos-position);

    vec4 viewPos = lightsBuffer.cameraView * vec4(position, 1.0f);

	int cascade = 0;

    for(int i = 0; i < SHADOW_CASCADES-1; i++)
        if (-viewPos.z > lightsBuffer.cascadeSplitDistances[i].x)
		    cascade = i+1;

    vec3 ambient = albedo * lightsBuffer.ambient;
    vec3 lighting = vec3(0.0);

    vec3 F0 = mix(vec3(0.04), albedo, metalness);

    vec3 viewDir = normalize(lightsBuffer.viewPos - position);
    float viewDist = length(lightsBuffer.viewPos - position);

    for(int i = 0; i < lightsBuffer.activePointLights; i++)
    {
        vec3 diff = position - lightsBuffer.pLights[i].position;
        float dist = length(diff);
        
        float shadow = 0.0;
        if(lightsBuffer.pLights[i].shadowmapIndex != -1)
            shadow = CalculateOmniShadows(diff, viewDist, lightsBuffer.pLights[i].radius, lightsBuffer.pLights[i].shadowmapIndex, lightsBuffer.pLights[i].pcfSampleRate, lightsBuffer.pLights[i].bias, lightsBuffer.pLights[i].shadowSoftness);
        
        if(dist < lightsBuffer.pLights[i].radius)
            lighting += CalculatePointLight(lightsBuffer.pLights[i], albedo, roughness, metalness, F0, viewDir, position, normal, dist) * (1.0-shadow);
    }
    for(int i = 0; i < lightsBuffer.activeSpotLights; i++)
    {
        vec3 lightToSurfaceDir = normalize(position - lightsBuffer.sLights[i].position);

        float innerCutoff = lightsBuffer.sLights[i].innerCutoff / 2.0 * PI;
        float outerCutoff = lightsBuffer.sLights[i].outerCutoff / 2.0 * PI;

        float angleDiff = RadiansBetweenDirs(lightsBuffer.sLights[i].direction, lightToSurfaceDir);
        float intensity = pow(min(1.0 - (angleDiff-innerCutoff) / (outerCutoff-innerCutoff), 1.0), 2.0);

        float shadow = 0.0;
        if(lightsBuffer.sLights[i].shadowmapIndex != -1)
        {
            vec4 fPosLightSpace = biasMat * lightsBuffer.sLights[i].projView * vec4(position, 1.0);
            shadow = CalculateShadow(fPosLightSpace, spotShadowmaps, lightsBuffer.sLights[i].shadowmapIndex, lightsBuffer.sLights[i].pcfSampleRate, lightsBuffer.sLights[i].bias, lightsBuffer.sLights[i].shadowSoftness);
        }
        
        if(angleDiff < outerCutoff)
            lighting += CalculateSpotLight(lightsBuffer.sLights[i], albedo, roughness, metalness, F0, viewDir, position, normal) * intensity * (1.0 - shadow);
    }
    for(int i = 0; i < lightsBuffer.activeDirLights; i++)
    {
        float shadow = 0.0;
        if(lightsBuffer.dLights[i].shadowmapIndex != -1)
        {
            vec4 fPosLightSpace = biasMat * lightsBuffer.dLights[i].projView[cascade] * vec4(position, 1.0);
            float softness = lightsBuffer.dLights[i].shadowSoftness / lightsBuffer.cascadeRatios[cascade].x;

            shadow = CalculateShadow(fPosLightSpace, dirShadowmaps, lightsBuffer.dLights[i].shadowmapIndex+cascade, lightsBuffer.dLights[i].pcfSampleRate, lightsBuffer.dLights[i].bias, softness);
        }
        
        lighting += CalculateDirLight(lightsBuffer.dLights[i], albedo, roughness, metalness, F0, viewDir, position, normal) * (1.0 - shadow);
    }

    vec3 result = vec3(0.0f);
    
    switch(lightsBuffer.debugMode)
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
            result = vec3(texture(dirShadowmaps, vec3(fTexcoord, 0)).r);
            break;
        case 7:
            switch(cascade)
            {
                case 0:
                    result = albedo + vec3(0.2, 0.0, 0.0);
                    break;
                case 1:
                    result = albedo + vec3(0.0, 0.2, 0.0);
                    break;
                case 2:
                    result = albedo + vec3(0.0, 0.0, 0.2);
                    break;
                default: 
                    result = vec3(0.1);
                    break;
            }
            break;
    }
    
    FragColor = vec4(result, 1.0);
}
#version 450

#include "../EruptionEngine.ini"

layout(location = 0) in vec4 fPosition;
layout(location = 1) in vec3 fNormal;
layout(location = 2) in vec2 fTexcoord;
layout(location = 3) in mat3 fTBN;

layout(location = 0) out vec4 FragColor;

layout(push_constant) uniform MaterialBufferObject
{
	layout(offset = 64) 
    vec3 color;
    float metalnessVal;
    float roughnessVal;
    float normalMultiplier;
} mbo;

layout(set = 1, binding = 1) uniform sampler2D albedoTexture;
layout(set = 1, binding = 2) uniform sampler2D roughnessTexture;
layout(set = 1, binding = 3) uniform sampler2D normalTexture;
layout(set = 1, binding = 4) uniform sampler2D metalnessTexture;

layout(set = 2, binding = 0) uniform samplerCubeArray pointShadowmaps;
layout(set = 2, binding = 1) uniform sampler2DArray spotShadowmaps;
layout(set = 2, binding = 2) uniform sampler2DArray dirShadowmaps;

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

layout(set = 2, binding = 3) uniform UBO 
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

    uvec4 tileCount;

    uvec4 tileSizes;

    float scale;
    float bias;

} lightsBuffer;

struct LightGrid
{
    uint offset;
    uint count;
};

layout (std430, set = 2, binding = 4) buffer pointLightIndexSSBO {
    uint globalPointLightIndexList[];
};
layout (std430, set = 2, binding = 5) buffer pointLightGridSSBO {
    LightGrid pointLightGrid[];
};

layout (std430, set = 2, binding = 6) buffer spotLightIndexSSBO {
    uint globalSpotLightIndexList[];
};
layout (std430, set = 2, binding = 7) buffer spotLightGridSSBO {
    LightGrid spotLightGrid[];
};


#define PI 3.14159265359


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
    vec2 texelSize = vec2(1.0) / 2048 * softness;
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

float CalculateOmniShadows(PointLight light, vec3 fragToLightDiff, float viewToFragDist)
{
    float shadow = 0.0;
    float currentDepth = length(fragToLightDiff);

    #if SOFT_SHADOWS
        for(int i = 0; i < 20; ++i)
        {
            for(int j = 1; j <= light.pcfSampleRate; j++)
            {
                float interpolator = float(j) / light.pcfSampleRate;
                float closestDepth = texture(pointShadowmaps, vec4(fragToLightDiff + pcfSampleOffsets[i] * light.shadowSoftness * interpolator * 0.01, light.shadowmapIndex)).r * light.radius;

                shadow += currentDepth - light.bias > closestDepth ? 1.0 : 0.0;
            }
        }
        shadow /= 20.0 * light.pcfSampleRate;  
    #else
        float closestDepth = texture(pointShadowmaps, vec4(fragToLightDiff, shadowmapIndex)).r * farPlane;

        shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    #endif

    return shadow;
}

vec3 CalculatePointLight(PointLight light, vec3 albedo, float roughness, float metalness, vec3 F0, vec3 viewDir, vec3 position, vec3 normal, vec3 diff)
{
    float dist = length(diff);

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

//float GetBlurredAO()
//{
//    vec2 texelSize = 1.0 / vec2(textureSize(SSAO, 0));
//
//    float result = 0.0;
//    for (int x = -2; x < 2; ++x) 
//    {
//        for (int y = -2; y < 2; ++y) 
//        {
//            vec2 offset = vec2(float(x), float(y)) * texelSize;
//            result += texture(SSAO, fTexcoord + offset).r;
//        }
//    }
//
//    return result / 16.0;
//}


vec3 NormalMapping()
{
    vec3 normalMap = texture(normalTexture, fTexcoord).xyz;
    normalMap = normalize(2.0 * normalMap - 1.0);
    
    vec3 result = normalize(fTBN * normalMap);
    result = mix(fNormal, result, mbo.normalMultiplier);

    return result;
}
void main() 
{
    vec4 albedoAlpha = texture(albedoTexture, fTexcoord);

    float alpha = albedoAlpha.a;

    vec3 albedo    = albedoAlpha.rgb * mbo.color;
    vec3 normal    = NormalMapping();
    vec3 position  = fPosition.xyz;

    float linearDepth = fPosition.w;

    float roughness = texture(roughnessTexture, fTexcoord).r * mbo.roughnessVal;
    float metalness = texture(metalnessTexture, fTexcoord).r * mbo.metalnessVal;
    
	int cascade = 0;

    for(int i = 0; i < SHADOW_CASCADES-1; i++)
        if (linearDepth > lightsBuffer.cascadeSplitDistances[i].x)
		    cascade = i+1;
    
    vec3 ambient = albedo * lightsBuffer.ambient;
    vec3 lighting = vec3(0.0);

    vec3 F0 = mix(vec3(0.04), albedo, metalness);

    vec3 viewDir = normalize(lightsBuffer.viewPos - position);
    float viewDist = length(lightsBuffer.viewPos - position);
    
    uint  zTile     = uint(max(log2(linearDepth) * lightsBuffer.scale + lightsBuffer.bias, 0.0));
    uvec3 tiles     = uvec3(uvec2(gl_FragCoord.xy / lightsBuffer.tileSizes.xy), zTile);
    uint  tileIndex = tiles.x +
                      lightsBuffer.tileCount.x * tiles.y +
                      lightsBuffer.tileCount.x * lightsBuffer.tileCount.y * tiles.z;  

    uint pointLightCount       = pointLightGrid[tileIndex].count;
    uint pointLightIndexOffset = pointLightGrid[tileIndex].offset;

    uint spotLightCount       = spotLightGrid[tileIndex].count;
    uint spotLightIndexOffset = spotLightGrid[tileIndex].offset;

    for(uint i = 0; i < pointLightCount; i++)
    {
        uint lightId = globalPointLightIndexList[pointLightIndexOffset + i];

        PointLight light = lightsBuffer.pLights[lightId];

        vec3 diff = position - light.position;

        float shadow = 0.0;
        if(light.shadowmapIndex != -1)
            shadow = CalculateOmniShadows(light, diff, viewDist);
        
        lighting += CalculatePointLight(light, albedo, roughness, metalness, F0, viewDir, position, normal, diff) * (1.0-shadow);
    }

    for(uint i = 0; i < spotLightCount; i++)
    {
        uint lightId = globalSpotLightIndexList[spotLightIndexOffset + i];

        SpotLight light = lightsBuffer.sLights[lightId];
        
        vec3 lightToSurfaceDir = normalize(position - light.position);
        
        float innerCutoff = light.innerCutoff / 2.0 * PI;
        float outerCutoff = light.outerCutoff / 2.0 * PI;
        
        float angleDiff = min(RadiansBetweenDirs(light.direction, lightToSurfaceDir), outerCutoff);
        float intensity = pow(min(1.0 - (angleDiff-innerCutoff) / (outerCutoff-innerCutoff), 1.0), 2.0);
        
        float shadow = 0.0;
        if(light.shadowmapIndex != -1)
        {
            vec4 fPosLightSpace = biasMat * light.projView * vec4(position, 1.0);
            shadow = CalculateShadow(fPosLightSpace, spotShadowmaps, light.shadowmapIndex, light.pcfSampleRate, light.bias, light.shadowSoftness);
        }
        
        lighting += CalculateSpotLight(light, albedo, roughness, metalness, F0, viewDir, position, normal) * intensity * (1.0 - shadow);
    }
    
    for(uint i = 0; i < lightsBuffer.activeDirLights; i++)
    {
        float shadow = 0.0;

        DirLight light = lightsBuffer.dLights[i];

        vec4 fPosLightSpace = biasMat * light.projView[cascade] * vec4(position, 1.0);
        float softness = light.shadowSoftness / lightsBuffer.cascadeRatios[cascade].x;

        if(light.shadowmapIndex != -1)
            shadow = CalculateShadow(fPosLightSpace, dirShadowmaps, light.shadowmapIndex+cascade, light.pcfSampleRate, light.bias, softness);
        
        lighting += CalculateDirLight(light, albedo, roughness, metalness, F0, viewDir, position, normal) * (1.0 - shadow);
    }
    
    vec3 result = vec3(0.0);

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
            result = ambient + vec3(float(pointLightCount) / (lightsBuffer.activePointLights+1), float(spotLightCount) / (lightsBuffer.activeSpotLights+1), 0);
            break;
        case 7:
            const vec3 depthSplitColors[8] = vec3[](
                vec3(0, 0, 0), vec3( 0, 0, 1), vec3(0, 1, 0), vec3(0, 1, 1),
                vec3(1, 0, 0), vec3( 1, 0, 1), vec3(1, 1, 0), vec3(1, 1, 1)
            );

            result = lighting + ambient + depthSplitColors[uint(zTile - (8.0 * floor(zTile/8.0)))] * 0.18;
            break;
        case 8:
            const vec3 cascadeColors[4] = vec3[](
                vec3(1, 0, 0), vec3( 0, 1, 0), vec3(0, 0, 1), vec3(0, 1, 1)
            );

            result = lighting + ambient + cascadeColors[uint(cascade - (4.0 * floor(cascade/4.0)))] * 0.18;
            break;
    }
    
    FragColor = vec4(result, 1.0);
}
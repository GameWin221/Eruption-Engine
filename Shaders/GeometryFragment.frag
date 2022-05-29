#version 450

layout(location = 0) in vec3 fPosition;
layout(location = 1) in vec3 fNormal;
layout(location = 2) in vec2 fTexcoord;
layout(location = 3) in mat3 fTBN;

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gPosition;
layout(location = 2) out vec4 gNormal;

layout(set = 1, binding = 1) uniform MatBufferObject {
    vec3 color;
    float metalnessVal;
    float roughnessVal;
    float normalMultiplier;
} mbo;

layout(set = 1, binding = 2) uniform sampler2D albedoTexture;
layout(set = 1, binding = 3) uniform sampler2D roughnessTexture;
layout(set = 1, binding = 4) uniform sampler2D normalTexture;
layout(set = 1, binding = 5) uniform sampler2D metalnessTexture;

vec3 TangentToWorld()
{
    vec3 normalMap = texture(normalTexture, fTexcoord).xyz;
    normalMap = normalize(2.0 * normalMap - 1.0);
    
    vec3 result = normalize(fTBN * normalMap);
    result = mix(fNormal, result, mbo.normalMultiplier);

    return result;
}
void main() 
{
    /// Output ///
    //gColor    - RGB: albedo            , A: roughness 
    //gPosition - RGB: frag position     , A: metalness
    //gNormal   - RGB: world space normal, A: nothing

    gColor = vec4(texture(albedoTexture, fTexcoord).rgb * mbo.color, texture(roughnessTexture, fTexcoord).g * mbo.roughnessVal);

    gPosition = vec4(fPosition, texture(metalnessTexture, fTexcoord).b * mbo.metalnessVal);

    gNormal = vec4(normalize(TangentToWorld()), 1.0f);
}
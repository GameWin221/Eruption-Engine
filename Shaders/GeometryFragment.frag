#version 450

layout(location = 0) in vec3 fNormal;
layout(location = 1) in vec2 fTexcoord;
layout(location = 2) in vec3 fPosition;
layout(location = 3) in vec3 fTangent;
layout(location = 4) in vec3 fBitangent;

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gPosition;
layout(location = 2) out vec4 gNormal;
layout(location = 3) out vec4 gTangent;

layout(set = 1, binding = 1) uniform MatBufferObject {
    vec3 color;
    float shininess;
    float normalStrength;
} mbo;

layout(set = 1, binding = 2) uniform sampler2D albedoTexture;
layout(set = 1, binding = 3) uniform sampler2D specularTexture;
layout(set = 1, binding = 4) uniform sampler2D normalTexture;

vec3 NormalMapping()
{
    vec3 normalMap = texture(normalTexture, fTexcoord).xyz;
    normalMap = normalize(2.0 * normalMap - 1.0);

    mat3 TBN = mat3(fTangent, fBitangent, fNormal);
    
    vec3 result = normalize(TBN * normalMap);

    result = mix(fNormal, result, mbo.normalStrength);

    return result;
}
void main() 
{
    gColor = vec4(texture(albedoTexture, fTexcoord).rgb * mbo.color, texture(specularTexture, fTexcoord).r);

    gPosition = vec4(fPosition, mbo.shininess);

    gNormal = vec4(NormalMapping(), 1.0);

    gTangent = vec4(fTangent, 1.0);
}
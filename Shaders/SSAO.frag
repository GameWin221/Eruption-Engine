#version 450

layout(location = 0) out float Occlusion;
layout(location = 0) in vec2 fTexcoord;

layout(set = 1, binding = 0) uniform sampler2D gPosition;
layout(set = 1, binding = 1) uniform sampler2D gNormal;
// Use depth buffer for less calculations

layout(set = 1, binding = 2) uniform UBO 
{
    vec3 kernels[64];
    vec3 rotations[16];
} ubo;

layout(set = 0, binding = 0) uniform CameraMatricesBufferObject
{
    mat4 view;
    mat4 proj;
} camera;

layout(push_constant) uniform SSAOParameters
{
    float screenWidth;
    float screenHeight;

    float radius;
    float bias;
    float multiplier;

} params;

void main()
{
    vec2 noiseScale = vec2(params.screenWidth/4.0, params.screenHeight/4.0);

    vec3 fragPos = (camera.view * vec4(texture(gPosition, fTexcoord).xyz, 1.0)).xyz;
    vec3 normal = (camera.view * vec4(normalize(texture(gNormal, fTexcoord).xyz), 0.0)).xyz;

    int x = int(floor(fTexcoord.x * noiseScale.x)) % 4;
    int y = int(floor(fTexcoord.y * noiseScale.y)) % 4;

    vec3 randomVec = normalize(ubo.rotations[y * 4 + x]);  

    vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normal);  

    float occlusion = 0.0;
    for(int i = 0; i < 64; ++i)
    {
        vec3 samplePos = TBN * ubo.kernels[i];
        samplePos = fragPos + samplePos * params.radius; 
        
        vec4 offset = vec4(samplePos, 1.0);
        offset      = camera.proj * offset;
        offset.xyz /= offset.w;
        offset.xyz  = offset.xyz * 0.5 + 0.5;  

        float sampleDepth = (camera.view * vec4(texture(gPosition, offset.xy).xyz, 1.0)).z; 

        float rangeCheck = smoothstep(0.0, 1.0, params.radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + params.bias ? 1.0 : 0.0) * rangeCheck * params.multiplier;   
    }  

    Occlusion = occlusion / 64;
}
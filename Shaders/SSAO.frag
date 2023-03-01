#version 450

layout(location = 0) out float Occlusion;
layout(location = 0) in vec2 fTexcoord;

layout(set = 1, binding = 0) uniform sampler2D Depth;

layout(set = 1, binding = 1) uniform UBO 
{
    vec3 kernels[64];
    vec3 rotations[16];
} ubo;

layout(set = 0, binding = 0) uniform CameraBufferObject
{
    mat4 view;
	mat4 invView;
	mat4 proj;
    mat4 invProj;
    mat4 invProjView;
	mat4 projView;

	uvec4 clusterTileCount;
	uvec4 clusterTileSizes;

	float clusterScale;
	float clusterBias;

    float zNear;
	float zFar;

    vec3 position;

	int debugMode;
} camera;

layout(push_constant) uniform SSAOParameters
{
    float screenWidth;
    float screenHeight;

    float radius;
    float bias;
    float multiplier;

} params;

float LinearDepth(float d)
{
    return camera.zNear * camera.zFar / (camera.zFar + d * (camera.zNear - camera.zFar));
}

vec3 WPosFromDepth(vec2 uv, float depth)
{
    vec2 ndc = uv * 2.0 - 1.0;

    vec4 worldSpace = camera.invProjView * vec4(ndc, depth, 1.0);

    return worldSpace.xyz / worldSpace.w;
}
vec3 VPosFromDepth(vec2 uv, float depth)
{
    vec2 ndc = uv * 2.0 - 1.0;

    vec4 viewSpace = camera.invProj * vec4(ndc, depth, 1.0);

    return viewSpace.xyz / viewSpace.w;
}

vec3 NormalFromDepth(float depth) 
{
    vec2 texelSize = 1.0 / textureSize(Depth, 0).xy;
    
    vec3 c = VPosFromDepth(fTexcoord, depth);

    // get view space position at 1 pixel offsets in each major direction
    vec3 r = VPosFromDepth(fTexcoord + vec2(texelSize.x, 0.0), texture(Depth, fTexcoord + vec2(texelSize.x, 0.0)).x);
    vec3 u = VPosFromDepth(fTexcoord + vec2(0.0, texelSize.y), texture(Depth, fTexcoord + vec2(0.0, texelSize.y)).x);

    // get the difference between the current and each offset position
    vec3 hDeriv = r - c;
    vec3 vDeriv = u - c;

    // get view space normal from the cross product of the diffs
    vec3 viewNormal = normalize(cross(hDeriv, vDeriv));
    viewNormal.z = -viewNormal.z;

    return viewNormal;

    /*

    vec2 texelSize = 1.0 / vec2(params.screenWidth, params.screenHeight);

    float c0 = depth;
    float l2 = texture(Depth, fTexcoord-vec2(2.0 * texelSize.x,0.0              )).x;
    float l1 = texture(Depth, fTexcoord-vec2(1.0 * texelSize.x,0.0              )).x;
    float r1 = texture(Depth, fTexcoord+vec2(1.0 * texelSize.x,0.0              )).x;
    float r2 = texture(Depth, fTexcoord+vec2(2.0 * texelSize.x,0.0              )).x;
    float b2 = texture(Depth, fTexcoord-vec2(0.0              ,2.0 * texelSize.y)).x;
    float b1 = texture(Depth, fTexcoord-vec2(0.0              ,1.0 * texelSize.y)).x;
    float t1 = texture(Depth, fTexcoord+vec2(0.0              ,1.0 * texelSize.y)).x;
    float t2 = texture(Depth, fTexcoord+vec2(0.0              ,2.0 * texelSize.y)).x;
    
    float dl = abs(l1*l2/(2.0*l2-l1)-c0);
    float dr = abs(r1*r2/(2.0*r2-r1)-c0);
    float db = abs(b1*b2/(2.0*b2-b1)-c0);
    float dt = abs(t1*t2/(2.0*t2-t1)-c0);
    
    vec3 ce = WPosFromDepth(fTexcoord, c0);

    vec3 dpdx = (dl<dr) ? ce-WPosFromDepth(fTexcoord-vec2(1.0 * texelSize.x, 0.0              ), l1): 
                         -ce+WPosFromDepth(fTexcoord+vec2(1.0 * texelSize.x, 0.0              ), r1);
    vec3 dpdy = (db<dt) ? ce-WPosFromDepth(fTexcoord-vec2(0.0              , 1.0 * texelSize.x), b1): 
                         -ce+WPosFromDepth(fTexcoord+vec2(0.0              , 1.0 * texelSize.x), t1);

    return normalize(cross(dpdx,dpdy));
    */
}

void main()
{
    vec2 noiseScale = vec2(params.screenWidth/4.0, params.screenHeight/4.0);

    float depth = texture(Depth, fTexcoord).x;

    vec3 fragPos = VPosFromDepth(fTexcoord, depth);

    vec3 normal = NormalFromDepth(depth);

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
        
        vec4 offset = camera.proj * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xyz  = offset.xyz * 0.5 + 0.5;  

        float sampleDepth = VPosFromDepth(offset.xy, depth).z; 

        float rangeCheck = smoothstep(0.0, 1.0, params.radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + params.bias ? 1.0 : 0.0) * rangeCheck * params.multiplier;   
    }  

    Occlusion = occlusion / 64;
}
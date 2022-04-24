#version 450

layout(location = 0) in vec2 fTexcoord;
layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform sampler2D HDRImage;

layout(push_constant) uniform exposurePushConstant
{
	float value;
} exposure;

void main() 
{
    vec3 hdrColor = texture(HDRImage, fTexcoord).rgb;
 
    vec3 ldrColor = vec3(1.0) - exp(-hdrColor * exposure.value);
  
    FragColor = vec4(ldrColor, 1.0);
}
#version 450

layout(location = 0) in vec2 fTexcoord;
layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform sampler2D HDRImage;

void main() 
{
    vec3 hdrColor = texture(HDRImage, fTexcoord).rgb;
 
    // Exposure
    const float exposure = 1.0;
    vec3 ldrColor = vec3(1.0) - exp(-hdrColor * exposure);
  
    FragColor = vec4(ldrColor, 1.0);
}
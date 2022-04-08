#version 450

layout(binding = 0) uniform sampler2D gColor;
layout(binding = 1) uniform sampler2D gPosition;
layout(binding = 2) uniform sampler2D gNormal;

layout(location = 0) in vec2 fTexcoord;

layout(location = 0) out vec4 FragColor;

const vec3 lightPos = vec3(2.0, 2.0, 2.0);

void main() 
{
    vec3 color = texture(gColor, fTexcoord).rgb;
    vec3 position = texture(gPosition, fTexcoord).rgb;
    vec3 normal = texture(gNormal, fTexcoord).rgb;

    float specularity = texture(gPosition, fTexcoord).a;

    vec3 lightDir = normalize(lightPos - position);  
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = vec3(diff);

    vec3 result = diffuse * color;

    FragColor = vec4(result, 1.0);
}
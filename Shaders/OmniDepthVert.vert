#version 450

#include "../EruptionEngine.ini"

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vTangent;
layout(location = 3) in vec2 vTexcoord;

layout(location = 0) out vec3 fPosition;
layout(location = 1) out vec3 lPos;
layout(location = 2) out float lFarPlane;

layout(push_constant) uniform PushConstant
{
	mat4x4 viewProj;
	mat4x4 model;
	// Last row of model is the light's position and far plane
} pc;

void main() 
{
	vec3 lightPos = vec3(pc.model[0][3], pc.model[1][3], pc.model[2][3]);
	float farPlane = pc.model[3][3];

	mat4 modelM = mat4(
		pc.model[0][0], pc.model[0][1], pc.model[0][2], 0.0, 
		pc.model[1][0], pc.model[1][1], pc.model[1][2], 0.0,
		pc.model[2][0], pc.model[2][1], pc.model[2][2], 0.0,
		pc.model[3][0], pc.model[3][1], pc.model[3][2], 1.0
	);

	vec4 worldPos = modelM * vec4(vPos, 1.0);

    gl_Position = pc.viewProj * worldPos;

	fPosition = worldPos.xyz;
	lPos = lightPos;
	lFarPlane = farPlane;
}
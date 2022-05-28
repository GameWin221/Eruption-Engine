#version 450

layout(binding = 0) uniform sampler2D AliasedImage;

layout(location = 0) in vec2 fTexcoord;
layout(location = 0) out vec4 AntialiasedResult;

layout(push_constant) uniform FXAAParameters
{
	float fxaaSpanMax;
	float fxaaReduceMin;
	float fxaaReduceMult;

	float power;

	vec2 texelSize;
} parameters;

void main() 
{
	vec3 luma = vec3(0.299, 0.587, 0.114);

	float lumaNW = dot(luma, texture(AliasedImage, fTexcoord + (vec2(-1.0, -1.0) * parameters.texelSize)).rgb);
	float lumaNE = dot(luma, texture(AliasedImage, fTexcoord + (vec2( 1.0, -1.0) * parameters.texelSize)).rgb);
	float lumaSW = dot(luma, texture(AliasedImage, fTexcoord + (vec2(-1.0,  1.0) * parameters.texelSize)).rgb);
	float lumaSE = dot(luma, texture(AliasedImage, fTexcoord + (vec2( 1.0,  1.0) * parameters.texelSize)).rgb);
	float lumaM  = dot(luma, texture(AliasedImage, fTexcoord).rgb);

	vec2 dir;
	dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
	dir.y =  ((lumaNW + lumaSE) - (lumaNE + lumaSE));

	float dirReduce = min((lumaNW + lumaNE + lumaSW + lumaSE) * (parameters.fxaaReduceMult * 0.25), parameters.fxaaReduceMin);
	float invDirAdjusment = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
	
	dir = min(vec2(parameters.fxaaSpanMax), max(vec2(-parameters.fxaaSpanMax), dir * invDirAdjusment)) * parameters.texelSize;

	dir *= parameters.power;

	vec3 result1 = 0.5 * (texture(AliasedImage, fTexcoord + (dir * (1.0/3.0 - 0.5))).rgb + 
						  texture(AliasedImage, fTexcoord + (dir * (2.0/3.0 - 0.5))).rgb);

	vec3 result2 = result1 * 0.5 + 0.25 * (
						  texture(AliasedImage, fTexcoord + (dir * -0.5)).rgb + 
						  texture(AliasedImage, fTexcoord + (dir *  0.5)).rgb);

	float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
	float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
	float lumaResult2 = dot(luma, result2);

	if(lumaResult2 < lumaMin || lumaResult2 > lumaMax)
		AntialiasedResult = vec4(result1, 1.0);
	else
		AntialiasedResult = vec4(result2, 1.0);
}
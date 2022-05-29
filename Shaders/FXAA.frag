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

const vec3 luma = vec3(0.299, 0.587, 0.114);

#define MODE_DIRECTION 2
#define MODE_EDGE_DETECT 1
#define MODE_NONE 0

#define DEBUG_MODE MODE_NONE

float RGBToLuminosity(vec3 rgb)
{
	return dot(luma, rgb);
}

void main() 
{
	// Getting color of the current pixel
	vec3 rgbM = texture(AliasedImage, fTexcoord).rgb;

	// Getting color of the 4 surrounding pixels in an X pattern
	vec3 rgbNW = texture(AliasedImage, fTexcoord + (vec2(-1.0, -1.0) * parameters.texelSize)).rgb;
	vec3 rgbNE = texture(AliasedImage, fTexcoord + (vec2( 1.0, -1.0) * parameters.texelSize)).rgb;
	vec3 rgbSW = texture(AliasedImage, fTexcoord + (vec2(-1.0,  1.0) * parameters.texelSize)).rgb;
	vec3 rgbSE = texture(AliasedImage, fTexcoord + (vec2( 1.0,  1.0) * parameters.texelSize)).rgb;

	// Calculating luminosity
	float lumaM  = RGBToLuminosity(rgbM );
	float lumaNW = RGBToLuminosity(rgbNW);
	float lumaNE = RGBToLuminosity(rgbNE);
	float lumaSW = RGBToLuminosity(rgbSW);
	float lumaSE = RGBToLuminosity(rgbSE);

	// Calculating the blur direction
	vec2 dir;
	dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
	dir.y =  ((lumaNW + lumaSE) - (lumaNE + lumaSE));

	// Adjusting the direction
	float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (parameters.fxaaReduceMult * 0.25), parameters.fxaaReduceMin);
	float invDirAdjusment = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
	
	dir = min(vec2(parameters.fxaaSpanMax), max(vec2(-parameters.fxaaSpanMax), dir * invDirAdjusment)) * parameters.texelSize;

	dir *= parameters.power;

	// Blurring
	vec3 result1 = 0.5 * (texture(AliasedImage, fTexcoord + dir * (1.0/3.0 - 0.5)).rgb + 
						  texture(AliasedImage, fTexcoord + dir * (2.0/3.0 - 0.5)).rgb);

	vec3 result2 = result1 * 0.5 + 0.25 * (
						  texture(AliasedImage, fTexcoord + dir * -0.5).rgb + 
						  texture(AliasedImage, fTexcoord + dir *  0.5).rgb);

	// Contrast based edge detection
	float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
	float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
	float lumaResult2 = RGBToLuminosity(result2);
	
	// Adjusting detected edges
	const float cutoff = 0.984;

	float filtered = lumaMax - lumaMin;
	filtered = clamp(filtered - (1.0 - cutoff), 0.0, 1.0);
	filtered *= (1.0 / cutoff);
	
	// Outputting the result
	#if (DEBUG_MODE == MODE_NONE)

	if(filtered < lumaMin || filtered > lumaMax)
		AntialiasedResult = vec4(rgbM, 1.0);
	else
		AntialiasedResult = vec4(result2, 1.0);
	
	#elif (DEBUG_MODE == MODE_EDGE_DETECT)

	if(filtered < lumaMin || filtered > lumaMax)
		AntialiasedResult = vec4(0.0, 1.0, 0.0, 1.0);
	else
		AntialiasedResult = vec4(1.0, 0.0, 0.0, 1.0);

	#elif (DEBUG_MODE == MODE_DIRECTION)

	AntialiasedResult = vec4(dir.x * 1.0 / parameters.texelSize.x, dir.y * 1.0 / parameters.texelSize.y, 0.0, 1.0);

	#endif
}
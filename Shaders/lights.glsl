#ifndef LIGHTS_GLSL
#define LIGHTS_GLSL
#include "../EruptionEngine.ini"

struct PointLight
{
    vec3 position;
    float radius;

    vec3 color;
	int shadowmapIndex;

	float shadowSoftness;
    int pcfSampleRate; 
    float bias;
    float _padding0;

    mat4 viewProj[6];
};
struct SpotLight
{
    vec3 position;
    float innerCutoff;

    vec3 direction;
    float outerCutoff;

    vec3 color;
    float range;

    mat4 viewProj;

    int shadowmapIndex;
    float shadowSoftness;
    int pcfSampleRate; 
    float bias;
};
struct DirLight
{
    vec3 direction;
    int shadowmapIndex;

    vec3 color;
    float shadowSoftness;

    int pcfSampleRate;
    float bias;
    float _padding0;
    float _padding1;
};
#endif
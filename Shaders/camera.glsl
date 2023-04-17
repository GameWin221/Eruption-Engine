#ifndef CAMERA_GLSL
#define CAMERA_GLSL

#include "../EruptionEngine.ini"

struct CameraBufferObject
{
    mat4 view;
	mat4 invView;
	mat4 proj;
    mat4 invProj;
    mat4 invProjView;
	mat4 projView;

	vec3 position;

	int debugMode;

	uvec4 clusterTileCount;
	uvec4 clusterTileSizes;

	vec4 cascadeSplitDistances[SHADOW_CASCADES];
	vec4 cascadeFrustumSizeRatios[SHADOW_CASCADES];

	mat4 cascadeMatrices[MAX_DIR_LIGHT_SHADOWS][SHADOW_CASCADES];

	float clusterScale;
	float clusterBias;

    float zNear;
	float zFar;
};

#endif
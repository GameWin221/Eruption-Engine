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

	float clusterScale;
	float clusterBias;

    float zNear;
	float zFar;
};
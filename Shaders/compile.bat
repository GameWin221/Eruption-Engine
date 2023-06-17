%VULKAN_SDK%/Bin/glslc.exe %~dp0\ForwardVert.vert -o %~dp0\ForwardVert.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\ForwardFrag.frag -o %~dp0\ForwardFrag.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\Depth.vert -o %~dp0\Depth.spv

%VULKAN_SDK%/Bin/glslc.exe %~dp0\ClusterAABB.comp -o %~dp0\ClusterAABB.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\ClusterLightCulling.comp -o %~dp0\ClusterLightCulling.spv

%VULKAN_SDK%/Bin/glslc.exe %~dp0\FullscreenTri.vert -o %~dp0\FullscreenTri.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\FXAA.frag -o %~dp0\FXAA.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\SSAO.frag -o %~dp0\SSAO.spv

%VULKAN_SDK%/Bin/glslc.exe %~dp0\PointShadowVert.vert -o %~dp0\PointShadowVert.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\SpotShadowVert.vert -o %~dp0\SpotShadowVert.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\DirShadowVert.vert -o %~dp0\DirShadowVert.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\ShadowFrag.frag -o %~dp0\ShadowFrag.spv

pause
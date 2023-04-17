%VULKAN_SDK%/Bin/glslc.exe %~dp0\vert.vert -o %~dp0\vert.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\frag.frag -o %~dp0\frag.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\Depth.vert -o %~dp0\Depth.spv

%VULKAN_SDK%/Bin/glslc.exe %~dp0\PointShadowVert.vert -o %~dp0\PointShadowVert.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\SpotShadowVert.vert -o %~dp0\SpotShadowVert.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\DirShadowVert.vert -o %~dp0\DirShadowVert.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\ShadowFrag.frag -o %~dp0\ShadowFrag.spv

pause
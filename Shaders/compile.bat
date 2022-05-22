%VULKAN_SDK%/Bin/glslc.exe %~dp0\GeometryVertex.vert -o %~dp0\GeometryVertex.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\GeometryFragment.frag -o %~dp0\GeometryFragment.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\FullscreenTriVert.vert -o %~dp0\FullscreenTriVert.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\LightingFrag.frag -o %~dp0\LightingFrag.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\TonemapFrag.frag -o %~dp0\TonemapFrag.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\DepthFragment.frag -o %~dp0\DepthFragment.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\DepthVertex.vert -o %~dp0\DepthVertex.spv

pause
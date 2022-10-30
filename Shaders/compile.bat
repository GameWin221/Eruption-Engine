%VULKAN_SDK%/Bin/glslc.exe %~dp0\GeometryVertex.vert -o %~dp0\GeometryVertex.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\GeometryFragment.frag -o %~dp0\GeometryFragment.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\FullscreenTriVert.vert -o %~dp0\FullscreenTriVert.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\Lighting.frag -o %~dp0\Lighting.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\Tonemapping.frag -o %~dp0\Tonemapping.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\Depth.vert -o %~dp0\Depth.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\FXAA.frag -o %~dp0\FXAA.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\OmniDepthVert.vert -o %~dp0\OmniDepthVert.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\OmniDepthFrag.frag -o %~dp0\OmniDepthFrag.spv
%VULKAN_SDK%/Bin/glslc.exe %~dp0\SSAO.frag -o %~dp0\SSAO.spv

pause
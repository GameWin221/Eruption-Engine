%VULKAN_SDK%/Bin/glslc.exe GeometryVertex.vert -o GeometryVertex.spv
%VULKAN_SDK%/Bin/glslc.exe GeometryFragment.frag -o GeometryFragment.spv
%VULKAN_SDK%/Bin/glslc.exe FullscreenTriVert.vert -o FullscreenTriVert.spv
%VULKAN_SDK%/Bin/glslc.exe LightingFrag.frag -o LightingFrag.spv
%VULKAN_SDK%/Bin/glslc.exe TonemapFrag.frag -o TonemapFrag.spv
%VULKAN_SDK%/Bin/glslc.exe DepthFragment.frag -o DepthFragment.spv
%VULKAN_SDK%/Bin/glslc.exe DepthVertex.vert -o DepthVertex.spv

pause
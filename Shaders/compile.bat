%VULKAN_SDK%/Bin/glslc.exe GeometryVertex.vert -o GeometryVertex.spv
%VULKAN_SDK%/Bin/glslc.exe GeometryFragment.frag -o GeometryFragment.spv
%VULKAN_SDK%/Bin/glslc.exe FullscreenTriVert.vert -o FullscreenTriVert.spv
%VULKAN_SDK%/Bin/glslc.exe LightingFrag.frag -o LightingFrag.spv
%VULKAN_SDK%/Bin/glslc.exe PostProcessFrag.frag -o PostProcessFrag.spv

pause
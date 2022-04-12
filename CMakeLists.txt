cmake_minimum_required(VERSION 3.10) 
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True) 

project(EruptionEngine VERSION 1.0)
find_package(glfw3 3.3 REQUIRED FATAL_ERROR)
find_package(OpenGL REQUIRED)
find_package(Vulkan REQUIRED FATAL_ERROR) 
FILE(GLOB MAINSOURCE Source/*.cpp)
FILE(GLOB RENDERERSOURCE Source/Rendering/*.cpp)
FILE(GLOB UTILITYSOURCE Source/Utility/*.cpp)
FILE(GLOB STBSOURCE External/STB/*.cpp)
FILE(GLOB TOBJSOURCE External/TOBJ/*.cpp) 

add_executable(EruptionEngine ${MAINSOURCE} ${RENDERERSOURCE} ${UTILITYSOURCE} ${STBSOURCE} ${TOBJSOURCE}) 

target_link_libraries(EruptionEngine GL)
target_link_libraries(EruptionEngine vulkan)
target_link_libraries(EruptionEngine glfw) 

target_include_directories(EruptionEngine 
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Source
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/External/ImGui
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/External/STB
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/External/GLM/include
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/External/TOBJ
	PUBLIC $ENV{VULKANSDK}/include 
)

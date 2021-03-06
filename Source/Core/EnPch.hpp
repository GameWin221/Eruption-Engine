#pragma once

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <string>
#include <map>
#include <unordered_map>
#include <array>
#include <chrono>
#include <memory>
#include <thread>
#include <algorithm>
#include <optional>
#include <fstream>
#include <set>

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

#include <Core/Log.hpp>

#define VULKAN_BACKEND 0
#define DIXECTX_BACKEND 1

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm.hpp>
#include <gtx/transform.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtx/hash.hpp>
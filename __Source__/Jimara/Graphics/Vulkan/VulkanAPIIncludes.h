#pragma once
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif __APPLE__
#ifndef VK_USE_PLATFORM_METAL_EXT
#define VK_USE_PLATFORM_METAL_EXT
#endif
#else
#define VK_USE_PLATFORM_WAYLAND_KHR
#define VK_USE_PLATFORM_XCB_KHR
#endif
#include <vulkan/vulkan.h>
#include <algorithm>

#ifndef min
using std::min;
#endif
#ifndef max
using std::max;
#endif

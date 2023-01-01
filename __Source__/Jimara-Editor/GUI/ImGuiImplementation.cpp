#include <Graphics/Vulkan/VulkanAPIIncludes.h>
#include "ImGuiIncludes.h"

#pragma warning(disable: 26812)
#pragma warning(disable: 26451)
#pragma warning(disable: 26495)
#pragma warning(disable: 28182)
#pragma warning(disable: 6011)
#include <imgui.cpp>
#include <imgui_demo.cpp>
#include <imgui_draw.cpp>
#include <imgui_tables.cpp>
#include <imgui_widgets.cpp>

#include <backends/imgui_impl_glfw.cpp>
#include <backends/imgui_impl_vulkan.cpp>

#include <implot.cpp>
#include <implot_demo.cpp>
#include <implot_items.cpp>

#ifdef IMGUI_ENABLE_FREETYPE
#include <misc/freetype/imgui_freetype.cpp>
#endif // IMGUI_ENABLE_FREETYPE

#pragma warning(default: 26812)
#pragma warning(default: 26451)
#pragma warning(default: 26495)
#pragma warning(default: 28182)
#pragma warning(default: 6011)

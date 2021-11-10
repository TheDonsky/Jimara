#include "ImGuiGLFWContext.h"

#pragma warning(disable: 26812)
#include <backends/imgui_impl_glfw.h>
#pragma warning(default: 26812)


namespace Jimara {
	namespace Editor {
		ImGuiGLFWContext::ImGuiGLFWContext(OS::GLFW_Window* window) : ImGuiWindowContext(window) {}

		ImGuiGLFWContext::~ImGuiGLFWContext() {
			std::shared_lock<std::shared_mutex> lock(OS::GLFW_Window::APILock());
			dynamic_cast<OS::GLFW_Window*>(Window())->ExecuteOnEventThread([]() {
				std::unique_lock<std::mutex> apiLock(ImGuiAPIContext::APILock());
				ImGui_ImplGlfw_Shutdown(); 
				});
		}

		void ImGuiGLFWContext::BeginFrame() {
			std::shared_lock<std::shared_mutex> lock(OS::GLFW_Window::APILock());
			dynamic_cast<OS::GLFW_Window*>(Window())->ExecuteOnEventThread([]() { 
				ImGui_ImplGlfw_NewFrame();
				});
			Size2 curSize = Window()->FrameBufferSize();
			if (curSize != m_lastSize) {
				ImGuiIO& io = ImGui::GetIO();// = ImVec2(curSize.x, curSize.y);
				//ImGui_ImplGlfw_
				m_lastSize = curSize;
			}
		}

		ImGuiGLFWVulkanContext::ImGuiGLFWVulkanContext(OS::GLFW_Window* window) : ImGuiWindowContext(window), ImGuiGLFWContext(window) {
			std::shared_lock<std::shared_mutex> lock(OS::GLFW_Window::APILock());
			window->ExecuteOnEventThread([&]() {
				std::unique_lock<std::mutex> apiLock(ImGuiAPIContext::APILock());
				ImGui_ImplGlfw_InitForVulkan(window->Handle(), true);
				});
		}
	}
}

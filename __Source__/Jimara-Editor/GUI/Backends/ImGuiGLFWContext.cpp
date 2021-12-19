#include "ImGuiGLFWContext.h"

#pragma warning(disable: 26812)
#include <backends/imgui_impl_glfw.h>
#pragma warning(default: 26812)


namespace Jimara {
	namespace Editor {
		ImGuiGLFWContext::ImGuiGLFWContext(ImGuiAPIContext* apiContext, OS::GLFW_Window* window) : ImGuiWindowContext(apiContext, window) {}

		ImGuiGLFWContext::~ImGuiGLFWContext() {
			ImGuiAPIContext::Lock guiLock(ImGuiContext());
			std::shared_lock<std::shared_mutex> lock(OS::GLFW_Window::APILock());
			dynamic_cast<OS::GLFW_Window*>(Window())->ExecuteOnEventThread([&]() {
				ImGui_ImplGlfw_Shutdown(); 
				});
		}

		void ImGuiGLFWContext::BeginFrame() {
			ImGuiAPIContext::Lock guiLock(ImGuiContext());
			std::shared_lock<std::shared_mutex> lock(OS::GLFW_Window::APILock());
			dynamic_cast<OS::GLFW_Window*>(Window())->ExecuteOnEventThread([&]() {
				ImGui_ImplGlfw_NewFrame();
				});
			Size2 curSize = Window()->FrameBufferSize();
			if (curSize != m_lastSize) {
				ImGuiIO& io = ImGui::GetIO();
				m_lastSize = curSize;
			}
		}

		void ImGuiGLFWContext::EndFrame() {
			ImGuiAPIContext::Lock guiLock(ImGuiContext());
			std::shared_lock<std::shared_mutex> lock(OS::GLFW_Window::APILock());
			dynamic_cast<OS::GLFW_Window*>(Window())->ExecuteOnEventThread([&]() {
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
				});
		}

		ImGuiGLFWVulkanContext::ImGuiGLFWVulkanContext(ImGuiAPIContext* apiContext, OS::GLFW_Window* window) 
			: ImGuiWindowContext(apiContext, window), ImGuiGLFWContext(apiContext, window) {
			ImGuiAPIContext::Lock guiLock(ImGuiContext());
			std::shared_lock<std::shared_mutex> lock(OS::GLFW_Window::APILock());
			window->ExecuteOnEventThread([&]() {
				ImGui_ImplGlfw_InitForVulkan(window->Handle(), true);
				});
		}
	}
}

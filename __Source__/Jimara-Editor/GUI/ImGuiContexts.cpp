#include "ImGuiAPIContext.h"
#include "Backends/ImGuiVulkanContext.h"
#include "Backends/ImGuiVulkanRenderer.h"
#include <cassert>


namespace Jimara {
	namespace Editor {
		namespace {
			inline static std::recursive_mutex& ApiLock() {
				static std::recursive_mutex apiLock;
				return apiLock;
			}
		}

		ImGuiAPIContext::ImGuiAPIContext(OS::Logger* logger) : m_logger(logger) {
			assert(m_logger != nullptr);
			std::unique_lock<std::recursive_mutex> lock(ApiLock());
			IMGUI_CHECKVERSION();
			m_context = ImGui::CreateContext();
			if (m_context != nullptr) {
				ImGui::SetCurrentContext(m_context);
				ImGui::StyleColorsDark();
				ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
				ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
				ImGui::GetIO().WantCaptureMouse = true;
				ImGui::GetIO().WantCaptureKeyboard = true;
				//ImGui::SetCurrentContext(nullptr);
			}
			else m_logger->Fatal("ImGuiAPIContext::ImGuiAPIContext - Failed to create context!");
		}

		ImGuiAPIContext::~ImGuiAPIContext() {
			std::unique_lock<std::recursive_mutex> lock(ApiLock());
			if (m_context != nullptr) {
				ImGui::SetCurrentContext(m_context);
				ImGui::DestroyContext(m_context);
				ImGui::SetCurrentContext(nullptr);
				m_context = nullptr;
			}
		}

		Reference<ImGuiDeviceContext> ImGuiAPIContext::CreateDeviceContext(Graphics::GraphicsDevice* device, OS::Window* window) {
			if (device == nullptr) {
				if (window != nullptr) window->Log()->Error("ImGuiAPIContext::CreateRenderer - null GraphicsDevice provided!");
				return nullptr;
			}
			else if (window == nullptr) {
				device->Log()->Error("ImGuiAPIContext::CreateRenderer - null Window provided!");
				return nullptr;
			}
			{
				Graphics::Vulkan::VulkanDevice* vulkanDevice = dynamic_cast<Graphics::Vulkan::VulkanDevice*>(device);
				if (vulkanDevice != nullptr)
					return Object::Instantiate<ImGuiVulkanContext>(this, vulkanDevice, window);
			}
			{
				device->Log()->Error("ImGuiAPIContext::CreateRenderer - Unknown GraphicsDevice type!");
				return nullptr;
			}
		}

		ImGuiAPIContext::Lock::Lock(const ImGuiAPIContext* context) : m_lock(ApiLock()), m_apiContext(context) {
			assert(m_apiContext != nullptr);
			m_oldContext = ImGui::GetCurrentContext();
			ImGui::SetCurrentContext(m_apiContext->m_context);
		}

		ImGuiAPIContext::Lock::~Lock() {
			assert(ImGui::GetCurrentContext() == m_apiContext->m_context);
			ImGui::SetCurrentContext(m_oldContext);
			m_lock.unlock();
		}
	}
}

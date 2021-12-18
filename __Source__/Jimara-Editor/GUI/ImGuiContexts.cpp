#include "ImGuiAPIContext.h"
#include "Backends/ImGuiVulkanContext.h"
#include "Backends/ImGuiVulkanRenderer.h"
#include <mutex>


namespace Jimara {
	namespace Editor {
		namespace {
			static std::mutex instanceLock;
			static ImGuiAPIContext* instance = nullptr;
		}

		ImGuiAPIContext::ImGuiAPIContext() {
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGui::StyleColorsDark();
			ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
			ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
			ImGui::GetIO().WantCaptureMouse = true;
			ImGui::GetIO().WantCaptureKeyboard = true;
		}

		ImGuiAPIContext::~ImGuiAPIContext() {
			ImGui::DestroyContext();
		}

		Reference<ImGuiAPIContext> ImGuiAPIContext::GetInstance() {
			std::unique_lock<std::mutex> lock(instanceLock);
			Reference<ImGuiAPIContext> ref = instance;
			if (ref == nullptr) {
				instance = ref = new ImGuiAPIContext();
				ref->ReleaseRef();
			}
			return ref;
		}

		void ImGuiAPIContext::OnOutOfScope()const {
			std::unique_lock<std::mutex> lock(instanceLock);
			if (RefCount() > 0) return;
			instance = nullptr;
			delete this;
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
					return Object::Instantiate<ImGuiVulkanContext>(vulkanDevice, window);
			}
			{
				device->Log()->Error("ImGuiAPIContext::CreateRenderer - Unknown GraphicsDevice type!");
				return nullptr;
			}
		}

		std::mutex& ImGuiAPIContext::APILock() {
			return instanceLock;
		}





		ImGuiDeviceContext::ImGuiDeviceContext(Graphics::GraphicsDevice* device) : m_apiContext(ImGuiAPIContext::GetInstance()), m_device(device) {}

		ImGuiWindowContext::ImGuiWindowContext(OS::Window* window) : m_apiContext(ImGuiAPIContext::GetInstance()), m_window(window) {}
	}
}

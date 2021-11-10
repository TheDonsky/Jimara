#include "ImGuiAPIContext.h"
#include "Backends/ImGuiVulkanContext.h"
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
			//ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
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

		namespace {
			class DeviceContextCache : public virtual ObjectCache<Reference<Graphics::GraphicsDevice>> {
			public:
				inline static Reference<ImGuiDeviceContext> GetDeviceContext(Graphics::GraphicsDevice* device) {
					if (device == nullptr) return nullptr;
					static DeviceContextCache cache;
					return cache.GetCachedOrCreate(device, false, [&]()->Reference<ImGuiDeviceContext> {
						{
							Graphics::Vulkan::VulkanDevice* vulkanDevice = dynamic_cast<Graphics::Vulkan::VulkanDevice*>(device);
							if (vulkanDevice != nullptr) return Object::Instantiate<ImGuiVulkanContext>(vulkanDevice);
						}
						{
							device->Log()->Error("ImGuiAPIContext::DeviceContextCache::GetDeviceContext - Unknown GraphicsDeviceType!");
							return nullptr;
						}
						});
				}
			};
		}

		Reference<ImGuiDeviceContext> ImGuiAPIContext::GetDeviceContext(Graphics::GraphicsDevice* device) {
			return DeviceContextCache::GetDeviceContext(device);
		}

		std::mutex& ImGuiAPIContext::APILock() {
			return instanceLock;
		}





		ImGuiDeviceContext::ImGuiDeviceContext(Graphics::GraphicsDevice* device) : m_apiContext(ImGuiAPIContext::GetInstance()), m_device(device) {}

		ImGuiWindowContext::ImGuiWindowContext(OS::Window* window) : m_apiContext(ImGuiAPIContext::GetInstance()), m_window(window) {}
	}
}

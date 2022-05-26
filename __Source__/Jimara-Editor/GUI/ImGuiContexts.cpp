#include "ImGuiAPIContext.h"
#include "Backends/ImGuiVulkanContext.h"
#include "Backends/ImGuiVulkanRenderer.h"
#include <Font-Awesome-Fonts/IconsMaterialDesign.h_MaterialIcons-Regular.ttf.h>
#include <Font-Awesome-Fonts/IconsFontAwesome5.h_fa-solid-900.ttf.h>
#include <Font-Awesome-Fonts/IconsFontaudio.h_fontaudio.ttf.h>
#include <IconFontCppHeaders/IconsMaterialDesign.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <IconFontCppHeaders/IconsFontaudio.h>
#include <cassert>
#include <string.h>


namespace Jimara {
	namespace Editor {
		namespace {
			inline static std::recursive_mutex& ApiLock() {
				static std::recursive_mutex apiLock;
				return apiLock;
			}

			inline static void AddFonts(OS::Logger* logger) {
				ImGuiIO& io = ImGui::GetIO();
				static const constexpr float FONT_SIZE = 13.0f;
				{
					ImFontConfig fontConfig = {};
					fontConfig.SizePixels = FONT_SIZE;
					fontConfig.OversampleH = fontConfig.OversampleV = 3;
					fontConfig.PixelSnapH = false;
					io.Fonts->AddFontDefault(&fontConfig);
				}
				static const ImFontConfig ICON_CONFIG = []() {
					ImFontConfig config = {};
					config.MergeMode = true;
					config.PixelSnapH = true;
					config.FontDataOwnedByAtlas = false;
					return config;
				}();
				{
					static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
					if (io.Fonts->AddFontFromMemoryTTF((void*)s_fa_solid_900_ttf, sizeof(s_fa_solid_900_ttf), FONT_SIZE, &ICON_CONFIG, icons_ranges) == nullptr)
						if (logger != nullptr) logger->Error("ImGuiAPIContext::ImGuiAPIContext - Failed to load s_fa_solid_900_ttf!");
				}
				{
					static const ImWchar icons_ranges[] = { ICON_MIN_FAD, ICON_MAX_FAD, 0 };
					if (io.Fonts->AddFontFromMemoryTTF((void*)s_fontaudio_ttf, sizeof(s_fontaudio_ttf), FONT_SIZE, &ICON_CONFIG, icons_ranges) == nullptr)
						if (logger != nullptr) logger->Error("ImGuiAPIContext::ImGuiAPIContext - Failed to load s_fontaudio_ttf!");
				}
				{
					static const ImWchar icons_ranges[] = { ICON_MIN_MD, ICON_MAX_MD, 0 };
					if (io.Fonts->AddFontFromMemoryTTF((void*)s_MaterialIcons_Regular_ttf, sizeof(s_MaterialIcons_Regular_ttf), FONT_SIZE, &ICON_CONFIG, icons_ranges) == nullptr)
						if (logger != nullptr) logger->Error("ImGuiAPIContext::ImGuiAPIContext - Failed to load s_MaterialIcons_Regular_ttf!");
				}
			}
		}

		ImGuiAPIContext::ImGuiAPIContext(OS::Logger* logger) : m_logger(logger) {
			assert(m_logger != nullptr);
			std::unique_lock<std::recursive_mutex> lock(ApiLock());
			ImGuiContext* oldContext = ImGui::GetCurrentContext();
			IMGUI_CHECKVERSION();
			m_context = ImGui::CreateContext();
			if (m_context != nullptr) {
				ImGui::SetCurrentContext(m_context);
				ImGui::StyleColorsDark();
				ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
				//ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
				ImGui::GetIO().WantCaptureMouse = true;
				ImGui::GetIO().WantCaptureKeyboard = true;
				AddFonts(logger);
				if (oldContext != nullptr)
					ImGui::SetCurrentContext(oldContext);
			}
			else m_logger->Fatal("ImGuiAPIContext::ImGuiAPIContext - Failed to create context!");
		}

		ImGuiAPIContext::~ImGuiAPIContext() {
			std::unique_lock<std::recursive_mutex> lock(ApiLock());
			if (m_context != nullptr) {
				ImGui::SetCurrentContext(m_context);
				ImGui::GetIO().Fonts->Clear();
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
			if (m_oldContext != nullptr)
				ImGui::SetCurrentContext(m_oldContext);
			m_lock.unlock();
		}
	}
}

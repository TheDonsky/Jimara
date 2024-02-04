#include "ImGuiAPIContext.h"
#include "../EditorWindows/Settings/ImGuiStyleEditor.h"
#include "Backends/ImGuiVulkanContext.h"
#include "Backends/ImGuiVulkanRenderer.h"
#include <Font-Awesome-Fonts/IconsMaterialDesign.h_MaterialIcons-Regular.ttf.h>
#include <Font-Awesome-Fonts/IconsFontAwesome5.h_fa-solid-900.ttf.h>
#include <Font-Awesome-Fonts/IconsFontaudio.h_fontaudio.ttf.h>
#include <FontHeaders/Cousine-Regular.h>
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
				static const constexpr float IMGUI_DEFAULT_FONT_SIZE = 13.0f;
				static const constexpr bool USE_IMGUI_DEFAULT_FONT = false;
				float fontSize = 15.0f;
				auto getFontConfig = [&]() {
					ImFontConfig config = {};
					config.SizePixels = fontSize;
					config.OversampleH = config.OversampleV = 4;
					//config.PixelSnapH = true;
					config.FontDataOwnedByAtlas = false;
					return config;
				};
				auto initDefaultFont = [&]() {
					fontSize = IMGUI_DEFAULT_FONT_SIZE;
					io.Fonts->AddFontDefault();
				};
				if (USE_IMGUI_DEFAULT_FONT)
					initDefaultFont();
				else {
					ImFontConfig config = getFontConfig();
					if (io.Fonts->AddFontFromMemoryCompressedTTF(COUSINE_REGULAR_compressed_data, COUSINE_REGULAR_compressed_size, fontSize, &config) == nullptr) {
						if (logger != nullptr) logger->Error("ImGuiAPIContext::ImGuiAPIContext - Failed to load Cousine-Regular.ttf!");
						initDefaultFont();
					}
				}
				const ImFontConfig ICON_CONFIG = [&]() {
					ImFontConfig config = getFontConfig();
					config.MergeMode = true;
					return config;
				}();
				{
					static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
					if (io.Fonts->AddFontFromMemoryTTF((void*)s_fa_solid_900_ttf, sizeof(s_fa_solid_900_ttf), fontSize, &ICON_CONFIG, icons_ranges) == nullptr)
						if (logger != nullptr) logger->Error("ImGuiAPIContext::ImGuiAPIContext - Failed to load s_fa_solid_900_ttf!");
				}
				{
					static const ImWchar icons_ranges[] = { ICON_MIN_FAD, ICON_MAX_FAD, 0 };
					if (io.Fonts->AddFontFromMemoryTTF((void*)s_fontaudio_ttf, sizeof(s_fontaudio_ttf), fontSize, &ICON_CONFIG, icons_ranges) == nullptr)
						if (logger != nullptr) logger->Error("ImGuiAPIContext::ImGuiAPIContext - Failed to load s_fontaudio_ttf!");
				}
				{
					static const ImWchar icons_ranges[] = { ICON_MIN_MD, ICON_MAX_MD, 0 };
					if (io.Fonts->AddFontFromMemoryTTF((void*)s_MaterialIcons_Regular_ttf, sizeof(s_MaterialIcons_Regular_ttf), fontSize, &ICON_CONFIG, icons_ranges) == nullptr)
						if (logger != nullptr) logger->Error("ImGuiAPIContext::ImGuiAPIContext - Failed to load s_MaterialIcons_Regular_ttf!");
				}
			}
		}

		ImGuiAPIContext::ImGuiAPIContext(OS::Logger* logger) : m_logger(logger) {
			assert(m_logger != nullptr);
			std::unique_lock<std::recursive_mutex> lock(ApiLock());
			ImGuiContext* oldContext = ImGui::GetCurrentContext();
			ImPlotContext* oldPlotContext = ImPlot::GetCurrentContext();
			IMGUI_CHECKVERSION();
			m_context = ImGui::CreateContext();
			if (m_context == nullptr) m_logger->Fatal("ImGuiAPIContext::ImGuiAPIContext - Failed to create context!");
			else {
				m_imPlotContext = ImPlot::CreateContext();
				if (m_imPlotContext == nullptr) {
					ImGui::DestroyContext(m_context);
					ImGui::SetCurrentContext(nullptr);
					m_context = nullptr;
					m_logger->Fatal("ImGuiAPIContext::ImGuiAPIContext - Failed to create ImPlot context!");
				}
				else {
					ImGui::SetCurrentContext(m_context);
					ImPlot::SetCurrentContext(m_imPlotContext);
					ImPlot::GetInputMap().Pan = 1;
					ImPlot::GetInputMap().Fit = 1;
					ImGui::StyleColorsDark();
					ImGuiStyleEditor::ApplyGammaToColors();
					ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
					//ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
					ImGui::GetIO().WantCaptureMouse = true;
					ImGui::GetIO().WantCaptureKeyboard = true;
					AddFonts(logger);
				}
			}
			if (oldContext != nullptr)
				ImGui::SetCurrentContext(oldContext);
			if (oldPlotContext != nullptr)
				ImPlot::SetCurrentContext(oldPlotContext);
		}

		ImGuiAPIContext::~ImGuiAPIContext() {
			std::unique_lock<std::recursive_mutex> lock(ApiLock());
			if (m_context != nullptr) {
				ImGui::SetCurrentContext(m_context);
				ImPlot::SetCurrentContext(m_imPlotContext);
				ImGui::GetIO().Fonts->Clear();
				if (m_imPlotContext != nullptr) {
					ImPlot::DestroyContext(m_imPlotContext);
					ImPlot::SetCurrentContext(nullptr);
				}
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
			m_oldPlotContext = ImPlot::GetCurrentContext();
			ImGui::SetCurrentContext(m_apiContext->m_context);
			ImPlot::SetCurrentContext(m_apiContext->m_imPlotContext);
		}

		ImGuiAPIContext::Lock::~Lock() {
			assert(ImGui::GetCurrentContext() == m_apiContext->m_context);
			assert(ImPlot::GetCurrentContext() == m_apiContext->m_imPlotContext);
			ImGui::SetCurrentContext(m_oldContext);
			ImPlot::SetCurrentContext(m_oldPlotContext);
			m_lock.unlock();
		}
	}
}

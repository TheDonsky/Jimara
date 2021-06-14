#include "VulkanRenderSurface.h"
#pragma warning(disable: 26812)

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanWindowSurface::VulkanWindowSurface(VulkanInstance* instance, OS::Window* window)
				: RenderSurface(instance), m_window(window) {
#ifdef _WIN32
				VkWin32SurfaceCreateInfoKHR createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
				createInfo.hwnd = m_window->GetHWND();
				createInfo.hinstance = GetModuleHandle(nullptr);
				if (vkCreateWin32SurfaceKHR(*instance, &createInfo, nullptr, &m_surface) != VK_SUCCESS)
					instance->Log()->Fatal("VulkanRenderSurface - Failed to create window surface!");
#elif __APPLE__
				m_surface = m_window->MakeVulkanSurface(*instance);
				/*
				PFN_vkCreateMetalSurfaceEXT vkCreateMetalSurfaceEXT =
					(PFN_vkCreateMetalSurfaceEXT)vkGetInstanceProcAddr(*instance, "vkCreateMetalSurfaceEXT");
				if (vkCreateMetalSurfaceEXT == nullptr)
					instance->Log()->Fatal("VulkanRenderSurface - Vulkan instance missing VK_EXT_metal_surface extension");
				VkMetalSurfaceCreateInfoEXT createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
				//createInfo.pLayer = window->GetMetalLayer();
				if(vkCreateMetalSurfaceEXT(*instance, &createInfo, nullptr, &m_surface) != VK_SUCCESS)
					instance->Log()->Fatal("VulkanRenderSurface - Failed to create window surface!");
				*/
#else
				VkXcbSurfaceCreateInfoKHR createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
				createInfo.connection = m_window->GetConnectionXCB();
				createInfo.window = m_window->GetWindowXCB();
				if (vkCreateXcbSurfaceKHR(*instance, &createInfo, nullptr, &m_surface) != VK_SUCCESS)
					instance->Log()->Fatal("VulkanRenderSurface - Failed to create window surface!");
#endif
				m_window->OnSizeChanged() += Callback<OS::Window*>(&VulkanWindowSurface::OnWindowSizeChanged, this);
			}

			VulkanWindowSurface::~VulkanWindowSurface() {
				m_window->OnSizeChanged() -= Callback<OS::Window*>(&VulkanWindowSurface::OnWindowSizeChanged, this);
				if (m_surface != VK_NULL_HANDLE) {
					vkDestroySurfaceKHR(*(static_cast<VulkanInstance*>(GraphicsInstance())), m_surface, nullptr);
					m_surface = VK_NULL_HANDLE;
				}
			}

			bool VulkanWindowSurface::DeviceCompatible(const PhysicalDevice* device)const {
				return DeviceCompatibilityInfo(this, dynamic_cast<const VulkanPhysicalDevice*>(device)).DeviceCompatible();
			}

			Size2 VulkanWindowSurface::Size()const {
				return m_window->FrameBufferSize();
			}

			VulkanWindowSurface::operator VkSurfaceKHR()const {
				return m_surface;
			}

			Event<VulkanWindowSurface*>& VulkanWindowSurface::OnSizeChanged() {
				return m_onSizeChanged;
			}

			std::shared_mutex& VulkanWindowSurface::ResizeLock()const {
				return m_window->MessageLock();
			}

			namespace {
				inline static std::optional<uint32_t> FindPresentQueueId(const VulkanWindowSurface* surface, const VulkanPhysicalDevice* device) {
					VkBool32 presentSupport = false;

					if (device->GraphicsQueueId().has_value()) {
						vkGetPhysicalDeviceSurfaceSupportKHR(*device, device->GraphicsQueueId().value(), *surface, &presentSupport);
						if (presentSupport)
							return device->GraphicsQueueId();
					}

					for (size_t i = 0; i < device->QueueFamilyCount(); i++) {
						vkGetPhysicalDeviceSurfaceSupportKHR(*device, static_cast<uint32_t>(i), *surface, &presentSupport);
						if (presentSupport)
							return static_cast<uint32_t>(i);
					}

					return std::optional<uint32_t>();
				}


				inline static size_t FindSurfaceFormats(const VulkanWindowSurface* surface, const VulkanPhysicalDevice* device, std::vector<VkSurfaceFormatKHR>& formats) {
					uint32_t formatCount;
					vkGetPhysicalDeviceSurfaceFormatsKHR(*device, *surface, &formatCount, nullptr);
					if (formatCount <= 0) return (~static_cast<size_t>(0));

					formats.resize(formatCount);
					vkGetPhysicalDeviceSurfaceFormatsKHR(*device, *surface, &formatCount, formats.data());

					std::optional<size_t> bestFormat;
					for (size_t i = 0; i < formats.size(); i++) {
						const VkSurfaceFormatKHR& format = formats[i];
						if (format.format == VK_FORMAT_B8G8R8A8_SRGB) {
							if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) return i;
							else if (!bestFormat.has_value()) bestFormat = i;
						}
					}
					return bestFormat.has_value() ? bestFormat.value() : 0;
				}

				inline static VkPresentModeKHR FindPresentModes(const VulkanWindowSurface* surface, const VulkanPhysicalDevice* device, std::unordered_set<VkPresentModeKHR>& presentModes) {
					uint32_t presentModeCount;
					vkGetPhysicalDeviceSurfacePresentModesKHR(*device, *surface, &presentModeCount, nullptr);
					if (presentModeCount <= 0) return VK_PRESENT_MODE_MAX_ENUM_KHR;

					std::vector<VkPresentModeKHR> availablePresentModes(presentModeCount);
					vkGetPhysicalDeviceSurfacePresentModesKHR(*device, *surface, &presentModeCount, availablePresentModes.data());
					presentModes = std::unordered_set<VkPresentModeKHR>(availablePresentModes.begin(), availablePresentModes.end());

					if (presentModes.find(VK_PRESENT_MODE_MAILBOX_KHR) != presentModes.end()) return VK_PRESENT_MODE_MAILBOX_KHR;
					else if (presentModes.find(VK_PRESENT_MODE_FIFO_RELAXED_KHR) != presentModes.end()) return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
					else if (presentModes.find(VK_PRESENT_MODE_IMMEDIATE_KHR) != presentModes.end()) return VK_PRESENT_MODE_IMMEDIATE_KHR;
					else return VK_PRESENT_MODE_FIFO_KHR;
				}
			}



			VulkanWindowSurface::DeviceCompatibilityInfo::DeviceCompatibilityInfo(const VulkanWindowSurface* surface, const VulkanPhysicalDevice* device)
				: m_capabilities{}, m_extent{ 0, 0 } {
				if (surface == nullptr || device == nullptr) return;

				m_presentQueueId = FindPresentQueueId(surface, device);
				if (!m_presentQueueId.has_value()) return;

				vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*device, *surface, &m_capabilities);

				size_t bestFormatId = FindSurfaceFormats(surface, device, m_surfaceFormats);
				if (bestFormatId < m_surfaceFormats.size()) m_preferredFormat = m_surfaceFormats[bestFormatId];

				VkPresentModeKHR presentMode = FindPresentModes(surface, device, m_presentModes);
				if (m_presentModes.size() > 0) m_preferredPresentMode = presentMode;

				if (m_capabilities.currentExtent.width != UINT32_MAX) m_extent = Size2(m_capabilities.currentExtent.width, m_capabilities.currentExtent.height);
				else {
					Size2 defaultSize = surface->Size();
					m_extent = Size2(
						std::max(m_capabilities.minImageExtent.width, std::min(m_capabilities.maxImageExtent.width, defaultSize.x)),
						std::max(m_capabilities.minImageExtent.height, std::min(m_capabilities.maxImageExtent.height, defaultSize.y))
					);
				}
			}

			bool VulkanWindowSurface::DeviceCompatibilityInfo::DeviceCompatible()const { return m_presentQueueId.has_value() && m_surfaceFormats.size() > 0 && m_presentModes.size() > 0; }

			uint32_t VulkanWindowSurface::DeviceCompatibilityInfo::PresentQueueId()const { return m_presentQueueId.value(); }

			VkSurfaceCapabilitiesKHR VulkanWindowSurface::DeviceCompatibilityInfo::Capabilities()const { return m_capabilities; }

			size_t VulkanWindowSurface::DeviceCompatibilityInfo::FormatCount()const { return m_surfaceFormats.size(); }

			VkSurfaceFormatKHR VulkanWindowSurface::DeviceCompatibilityInfo::Format(size_t index)const { return m_surfaceFormats[index]; }

			VkSurfaceFormatKHR VulkanWindowSurface::DeviceCompatibilityInfo::PreferredFormat()const { return m_preferredFormat.value(); }

			bool VulkanWindowSurface::DeviceCompatibilityInfo::SupportsPresentMode(VkPresentModeKHR mode)const { return m_presentModes.find(mode) != m_presentModes.end(); }

			VkPresentModeKHR VulkanWindowSurface::DeviceCompatibilityInfo::PreferredPresentMode()const { return m_preferredPresentMode.value(); }

			Size2 VulkanWindowSurface::DeviceCompatibilityInfo::Extent()const { return m_extent; }

			uint32_t VulkanWindowSurface::DeviceCompatibilityInfo::DefaultImageCount()const {
				uint32_t count = m_capabilities.minImageCount + 1;
				if (m_capabilities.maxImageCount > 0 && count > m_capabilities.maxImageCount)
					count = m_capabilities.maxImageCount;
				return count;
			}
		}
	}
}

#pragma warning(default: 26812)

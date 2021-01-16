#pragma once
#include "../VulkanInstance.h"
#include "../VulkanPhysicalDevice.h"
#include <optional>

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary> Vulkan window surface </summary>
			class VulkanWindowSurface : public RenderSurface {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="instance"> Vulkan API instance </param>
				/// <param name="window"> Target window </param>
				VulkanWindowSurface(VulkanInstance* instance, OS::Window* window);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanWindowSurface();

				/// <summary>
				/// Tells, if given physical device can draw on the surface or not
				/// </summary>
				/// <param name="device"> Device to check </param>
				/// <returns> True, if the device is compatible with the surface </returns>
				virtual bool DeviceCompatible(const PhysicalDevice* device)const override;

				/// <summary> Size of the surface (in pixels) </summary>
				virtual glm::uvec2 Size()const override;

				/// <summary> Type cast to API object </summary>
				operator VkSurfaceKHR()const;

				/// <summary> Information about device compatability </summary>
				class DeviceCompatibilityInfo {
				public:
					/// <summary>
					/// Constructor
					/// </summary>
					/// <param name="surface"> Surface to check </param>
					/// <param name="device"> Device to check </param>
					DeviceCompatibilityInfo(const VulkanWindowSurface* surface = nullptr, const VulkanPhysicalDevice* device = nullptr);

					/// <summary> True if device compatible </summary>
					bool DeviceCompatible()const;

					/// <summary> Index of the vulkan queue that supports presentation on given device (valid if and only if the device is compatible) </summary>
					uint32_t PresentQueueId()const;

					/// <summary> Surface capabilities (valid if and only if the device is compatible) </summary>
					VkSurfaceCapabilitiesKHR Capabilities()const;

					/// <summary> Number of available formats (valid if and only if the device is compatible) </summary>
					size_t FormatCount()const;

					/// <summary>
					/// Available format by index
					/// </summary>
					/// <param name="index"> Format index (0 - FormatCount) </param>
					/// <returns> Supported format </returns>
					VkSurfaceFormatKHR Format(size_t index)const;

					/// <summary> Preffered format to be used with the surface (valid if and only if the device is compatible) </summary>
					VkSurfaceFormatKHR PreferredFormat()const;

					/// <summary>
					/// Tells if given present mode is supported
					/// </summary>
					/// <param name="mode"> Mode to check </param>
					/// <returns> True if the mode is supported </returns>
					bool SupportsPresentMode(VkPresentModeKHR mode)const;

					/// <summary> Preffered present mode to be used with the surface (valid if and only if the device is compatible) </summary>
					VkPresentModeKHR PreferredPresentMode()const;

					/// <summary> Swap chain size for the surface </summary>
					VkExtent2D Extent()const;

					/// <summary> Default image count for the swap chain </summary>
					uint32_t DefaultImageCount()const;


				private:
					// Present queue
					std::optional<uint32_t> m_presentQueueId;

					// Capabilities
					VkSurfaceCapabilitiesKHR m_capabilities;

					// Available formats
					std::vector<VkSurfaceFormatKHR> m_surfaceFormats;

					// Best format
					std::optional<VkSurfaceFormatKHR> m_preferredFormat;

					// Available present modes
					std::unordered_set<VkPresentModeKHR> m_presentModes;

					// Best present mode
					std::optional<VkPresentModeKHR> m_preferredPresentMode;
					
					// Swap chain image size
					VkExtent2D m_extent;
				};

			private:
				// Target window
				Reference<OS::Window> m_window;

				// Underlying surface
				VkSurfaceKHR m_surface;
			};
		}
	}
}

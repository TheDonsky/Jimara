#pragma once
#include "VulkanRenderSurface.h"
#include "../VulkanDevice.h"
#include "../Memory/VulkanTexture.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Vulkan swap chain based on a window surface
			/// </summary>
			class VulkanSwapChain : public virtual Object {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> "Owner" device </param>
				/// <param name="target"> Target surface </param>
				VulkanSwapChain(VulkanDevice* device, VulkanWindowSurface* target);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanSwapChain();

				/// <summary> Type cast to API object </summary>
				operator VkSwapchainKHR()const;

				/// <summary> Swap chain image count </summary>
				size_t ImageCount()const;

				/// <summary>
				/// Swap chain image by id
				/// </summary>
				/// <param name="index"> Id of the swap chain image </param>
				/// <returns> Image </returns>
				VulkanImage* Image(size_t index)const;

				/// <summary> Surface format </summary>
				VkSurfaceFormatKHR Format()const;

				/// <summary> Surface size </summary>
				VkExtent2D Size()const;

				/// <summary> Present queue </summary>
				VkQueue PresentQueue()const;



			private:
				// "Owner" device
				Reference<VulkanDevice> m_device;

				// Target surface
				Reference<VulkanWindowSurface> m_surface;

				// Device-surface compatibility information
				VulkanWindowSurface::DeviceCompatibilityInfo m_compatibilityInfo;

				// Underlying API object
				VkSwapchainKHR m_swapChain;
				
				// Swap chain images
				std::vector<Reference<VulkanImage>> m_images;

				// Present queue
				VkQueue m_presentQueue;
			};
		}
	}
}

#pragma once
#include "VulkanRenderSurface.h"
#include "../VulkanDevice.h"
#include "../Memory/Textures/VulkanImage.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Vulkan swap chain based on a window surface
			/// </summary>
			class JIMARA_API VulkanSwapChain : public virtual Object {
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
				Size2 Size()const;

				/// <summary> "Owner" device </summary>
				VulkanDevice* Device()const;

				/// <summary>
				/// Aquires next image and returns it's index
				/// </summary>
				/// <param name="imageAvailableSemaphore"> Semaphore to be signalled when image becomes available </param>
				/// <param name="index"> Index of the image will be stored here </param>
				/// <param name="image"> Image reference will be stored here </param>
				/// <returns> True, if the image got aquired successfully (false means that swap chain has to be recreated) </returns>
				bool AquireNextImage(VkSemaphore imageAvailableSemaphore, size_t& index, VulkanImage*& image)const;

				/// <summary>
				/// Presents image to the surface
				/// </summary>
				/// <param name="imageId"> Image index </param>
				/// <param name="renderFinishedSemaphore"> Semaphore to wait for before presenting the image </param>
				/// <returns> True, if the image got presented successfully (false means that swap chain has to be recreated) </returns>
				bool Present(size_t imageId, VkSemaphore renderFinishedSemaphore)const;



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
				Reference<VulkanDeviceQueue> m_presentQueue;
			};
		}
	}
}

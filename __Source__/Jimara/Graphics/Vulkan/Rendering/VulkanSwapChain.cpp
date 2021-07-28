#include "VulkanSwapChain.h"
#include <sstream>


#pragma warning(disable: 26812)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			namespace {
				class SwapChainImage : public virtual VulkanStaticImage {
				private:
					const VulkanSwapChain* m_swapChain;
					const VkImage m_image;

				public:
					SwapChainImage(const VulkanSwapChain* swapChain, VkImage image) : m_swapChain(swapChain), m_image(image) {}

					virtual operator VkImage()const override { return m_image; }

					virtual VkFormat VulkanFormat()const override { return m_swapChain->Format().format; }

					virtual VulkanDevice* Device()const override { return m_swapChain->Device(); }

					virtual TextureType Type()const override { return TextureType::TEXTURE_2D; }
					
					virtual PixelFormat ImageFormat()const { return VulkanImage::PixelFormatFromNativeFormat(VulkanFormat()); }

					virtual Multisampling SampleCount()const override { return Multisampling::SAMPLE_COUNT_1; }

					virtual Size3 Size()const override { Size2 size = m_swapChain->Size(); return Size3(size.x, size.y, 1); }

					virtual uint32_t ArraySize()const override { return 1; }

					virtual uint32_t MipLevels()const override { return 1; }
				};
			}

			VulkanSwapChain::VulkanSwapChain(VulkanDevice* device, VulkanWindowSurface* surface)
				: m_device(device), m_surface(surface), m_compatibilityInfo(surface, device->PhysicalDeviceInfo())
				, m_swapChain(VK_NULL_HANDLE), m_presentQueue(VK_NULL_HANDLE) {
				if (!m_compatibilityInfo.DeviceCompatible())
					m_device->Log()->Fatal("VulkanSwapChain - Surface and device are not compatible");

				vkGetDeviceQueue(*m_device, m_compatibilityInfo.PresentQueueId(), 0, &m_presentQueue);

				VkSwapchainCreateInfoKHR createInfo = {};
				{
					createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
					createInfo.surface = *m_surface;
					createInfo.minImageCount = m_compatibilityInfo.DefaultImageCount();
					createInfo.imageFormat = m_compatibilityInfo.PreferredFormat().format;
					createInfo.imageColorSpace = m_compatibilityInfo.PreferredFormat().colorSpace;
					Size2 extent = m_compatibilityInfo.Extent();
					createInfo.imageExtent = { extent.x, extent.y };
					createInfo.imageArrayLayers = 1;
					createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
						| VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
						| VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
				}

				uint32_t graphicsFamily = m_device->PhysicalDeviceInfo()->GraphicsQueueId().value();
				uint32_t queueFamilyIndices[] = { graphicsFamily, m_compatibilityInfo.PresentQueueId() };
				if (graphicsFamily != m_compatibilityInfo.PresentQueueId()) {
					createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
					createInfo.queueFamilyIndexCount = 2;
					createInfo.pQueueFamilyIndices = queueFamilyIndices;
				}
				else {
					createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
					createInfo.queueFamilyIndexCount = 0; // Optional
					createInfo.pQueueFamilyIndices = nullptr; // Optional
				}

				{
					createInfo.preTransform = m_compatibilityInfo.Capabilities().currentTransform;
					createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
					createInfo.presentMode = m_compatibilityInfo.PreferredPresentMode();
					createInfo.clipped = VK_TRUE;
					createInfo.oldSwapchain = VK_NULL_HANDLE;
				}

				if (vkCreateSwapchainKHR(*m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
					m_device->Log()->Fatal("VulkanSwapChain - Failed to create swap chain!");

				uint32_t imageCount = 0;
				vkGetSwapchainImagesKHR(*m_device, m_swapChain, &imageCount, nullptr);
				std::vector<VkImage> swapChainImages(imageCount);
				vkGetSwapchainImagesKHR(*m_device, m_swapChain, &imageCount, swapChainImages.data());

				for (size_t i = 0; i < swapChainImages.size(); i++)
					m_images.push_back(Object::Instantiate<SwapChainImage>(this, swapChainImages[i]));

#ifndef NDEBUG
				{
					std::stringstream stream;
					stream << "VulkanSwapChain: Swap chain instantiated:" << std::endl
						<< "    SURFACE FORMAT: {" << m_compatibilityInfo.PreferredFormat().format << "; " << m_compatibilityInfo.PreferredFormat().colorSpace << "}" << std::endl
						<< "    PRESENT MODE:   " << m_compatibilityInfo.PreferredPresentMode() << std::endl
						<< "    EXTENST:        (" << m_compatibilityInfo.Extent().x << " * " << m_compatibilityInfo.Extent().y << ")" << std::endl
						<< "    IMAGE COUNT:    " << swapChainImages.size() << std::endl
						<< "    GRAPHICS QUEUE: " << m_device->PhysicalDeviceInfo()->GraphicsQueueId().value() << std::endl
						<< "    PRESENT QUEUE:  " << m_compatibilityInfo.PresentQueueId() << std::endl;
					m_device->Log()->Info(stream.str());
				}
#endif
			}

			VulkanSwapChain::~VulkanSwapChain() {
				vkDeviceWaitIdle(*m_device);
				m_images.clear();
				if (m_swapChain != VK_NULL_HANDLE) {
					vkDestroySwapchainKHR(*m_device, m_swapChain, nullptr);
					m_swapChain = VK_NULL_HANDLE;
				}
			}

			VulkanSwapChain::operator VkSwapchainKHR()const { return m_swapChain; }

			size_t VulkanSwapChain::ImageCount()const { return m_images.size(); }

			VulkanImage* VulkanSwapChain::Image(size_t index)const { return m_images[index]; }

			VkSurfaceFormatKHR VulkanSwapChain::Format()const { return m_compatibilityInfo.PreferredFormat(); }

			Size2 VulkanSwapChain::Size()const { return m_compatibilityInfo.Extent(); }

			VkQueue VulkanSwapChain::PresentQueue()const { return m_presentQueue; }

			VulkanDevice* VulkanSwapChain::Device()const { return m_device; }

			bool VulkanSwapChain::AquireNextImage(VkSemaphore imageAvailableSemaphore, size_t& index, VulkanImage*& image)const {
				uint32_t imageId;
				while (true) {
					VkResult result = vkAcquireNextImageKHR(*m_device, m_swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageId);
					if (result == VK_ERROR_OUT_OF_DATE_KHR) return false;
					else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
						m_device->Log()->Fatal("VulkanSwapChain - Failed to acquire swap chain image!");
					else break;
				}
				index = static_cast<uint32_t>(imageId);
				image = m_images[index];
				return true;
			}

			bool VulkanSwapChain::Present(size_t imageId, VkSemaphore renderFinishedSemaphore)const {
				VkPresentInfoKHR presentInfo = {};
				presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

				presentInfo.waitSemaphoreCount = (renderFinishedSemaphore != VK_NULL_HANDLE ? 1 : 0);
				presentInfo.pWaitSemaphores = &renderFinishedSemaphore;

				presentInfo.swapchainCount = 1;
				presentInfo.pSwapchains = &m_swapChain;
				uint32_t imageIndex = static_cast<uint32_t>(imageId);
				presentInfo.pImageIndices = &imageIndex;
				presentInfo.pResults = nullptr;

				VkResult result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
				if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
					return false;
				else if (result != VK_SUCCESS) {
					m_device->Log()->Fatal("VulkanSwapChain - Failed to present swap chain image!");
					return false;
				}
				else return true;
			}
		}
	}
}

#pragma warning(default: 26812)

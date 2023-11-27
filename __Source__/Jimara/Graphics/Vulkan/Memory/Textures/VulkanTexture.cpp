#include "VulkanTexture.h"


#pragma warning(disable: 26812)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanTexture::VulkanTexture(
				VulkanDevice* device, TextureType type, PixelFormat format, Size3 size, uint32_t arraySize, bool generateMipmaps,
				VkImageUsageFlags usage, Multisampling sampleCount, VkMemoryPropertyFlags memoryFlags, VkImageLayout shaderAccessLayout)
				: VulkanImage(shaderAccessLayout)
				, m_device(device), m_textureType(type), m_pixelFormat(format), m_textureSize(size), m_arraySize(arraySize)
				, m_mipLevels(generateMipmaps ? CalculateSupportedMipLevels(device, format, size) : 1u), m_sampleCount(sampleCount)
				, m_updateCache(device) {

				VkImageTiling tiling;
				VkImageLayout initialLayout;
				if ((memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
					tiling = VK_IMAGE_TILING_LINEAR;
					initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				}
				else {
					tiling = VK_IMAGE_TILING_OPTIMAL;
					initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
				}

				VkImageUsageFlags use = 0;
				usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
				for (VkImageUsageFlags flag = 1; flag <= usage; flag <<= 1) 
					if ((flag & usage) != 0) {
						VkImageFormatProperties props;
						if (vkGetPhysicalDeviceImageFormatProperties(*device->PhysicalDeviceInfo()
							, NativeFormatFromPixelFormat(m_pixelFormat)
							, NativeTypeFromTextureType(m_textureType)
							, tiling, flag, 0, &props) == VK_SUCCESS) use |= flag;
					}

				VkImageCreateInfo imageInfo = {};
				{
					imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
					imageInfo.imageType = NativeTypeFromTextureType(m_textureType);
					imageInfo.extent.width = m_textureSize.x;
					imageInfo.extent.height = m_textureSize.y;
					imageInfo.extent.depth = m_textureSize.z;
					imageInfo.mipLevels = m_mipLevels;
					imageInfo.arrayLayers = m_arraySize;
					imageInfo.format = NativeFormatFromPixelFormat(m_pixelFormat);
					imageInfo.tiling = tiling;
					imageInfo.initialLayout = initialLayout;
					imageInfo.usage = use;
					imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
					imageInfo.samples = device->PhysicalDeviceInfo()->SampleCountFlags(m_sampleCount);
					imageInfo.flags = 0; // Optional
				}
				if (vkCreateImage(*m_device, &imageInfo, nullptr, &m_image) != VK_SUCCESS)
					m_device->Log()->Fatal("VulkanTexture - Failed to create image!");

				VkMemoryRequirements memRequirements;
				vkGetImageMemoryRequirements(*m_device, m_image, &memRequirements);

				m_memory = m_device->MemoryPool()->Allocate(memRequirements, memoryFlags);
				vkBindImageMemory(*m_device, m_image, m_memory->Memory(), m_memory->Offset());

				assert(ShaderAccessLayout() == shaderAccessLayout);
				auto transitionLayout = [&](CommandBuffer* buffer) {
					TransitionLayout(dynamic_cast<VulkanCommandBuffer*>(buffer),
						VK_IMAGE_LAYOUT_UNDEFINED, ShaderAccessLayout(), 0, MipLevels(), 0, ArraySize());
				};
				if ((memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
					assert((m_memory->Flags() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0);
					const VulkanDevice::OneTimeCommandBufferInfo info = 
						m_device->SubmitOneTimeCommandBuffer(Callback<VulkanPrimaryCommandBuffer*>::FromCall(&transitionLayout));
					m_initialLayoutTransition = info.commandBuffer;
				}
				else m_updateCache.Execute(transitionLayout);
			}

			VulkanTexture::~VulkanTexture() {
				WaitTillMemoryCanBeMapped();
				m_updateCache.Clear();

				if (m_image != VK_NULL_HANDLE) {
					vkDestroyImage(*m_device, m_image, nullptr);
					m_image = VK_NULL_HANDLE;
				}
			}

			Texture::TextureType VulkanTexture::Type()const {
				return m_textureType;
			}

			Texture::PixelFormat VulkanTexture::ImageFormat()const {
				return m_pixelFormat;
			}

			Texture::Multisampling VulkanTexture::SampleCount()const {
				return m_sampleCount;
			}

			Size3 VulkanTexture::Size()const {
				return m_textureSize;
			}

			uint32_t VulkanTexture::ArraySize()const {
				return m_arraySize;
			}

			uint32_t VulkanTexture::MipLevels()const {
				return m_mipLevels;
			}

			VulkanTexture::operator VkImage()const {
				return m_image;
			}

			VkFormat VulkanTexture::VulkanFormat()const {
				return NativeFormatFromPixelFormat(m_pixelFormat);
			}

			VulkanDevice* VulkanTexture::Device()const {
				return m_device;
			}

			bool VulkanTexture::WaitTillMemoryCanBeMapped() {
				if ((m_memory->Flags() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
					return false;
				std::unique_lock<decltype(m_initialLayoutTransitionLock)> lock(m_initialLayoutTransitionLock);
				if (m_initialLayoutTransition != nullptr) {
					m_initialLayoutTransition->Wait();
					m_initialLayoutTransition = nullptr;
				}
				return true;
			}
		}
	}
}
#pragma warning(default: 26812)

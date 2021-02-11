#include "VulkanStaticTexture.h"


#pragma warning(disable: 26812)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanStaticTexture::VulkanStaticTexture(
				VulkanDevice* device, TextureType type, PixelFormat format, Size3 size, uint32_t arraySize, bool generateMipmaps,
				VkImageUsageFlags usage, Multisampling sampleCount)
				: m_device(device), m_textureType(type), m_pixelFormat(format), m_textureSize(size), m_arraySize(arraySize)
				, m_mipLevels(generateMipmaps ? CalculateSupportedMipLevels(device, format, size) : 1u), m_sampleCount(sampleCount) {

				VkImageUsageFlags use = 0;
				usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
				for (VkImageUsageFlags flag = 1; flag <= usage; flag <<= 1) 
					if ((flag & usage) != 0) {
						VkImageFormatProperties props;
						if (vkGetPhysicalDeviceImageFormatProperties(*device->PhysicalDeviceInfo()
							, NativeFormatFromPixelFormat(m_pixelFormat)
							, NativeTypeFromTextureType(m_textureType)
							, VK_IMAGE_TILING_OPTIMAL
							, flag, 0, &props) == VK_SUCCESS) use |= flag;
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
					imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
					imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					imageInfo.usage = use;
					imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
					imageInfo.samples = device->PhysicalDeviceInfo()->SampleCountFlags(m_sampleCount);
					imageInfo.flags = 0; // Optional
				}
				if (vkCreateImage(*m_device, &imageInfo, nullptr, &m_image) != VK_SUCCESS)
					m_device->Log()->Fatal("VulkanTexture - Failed to create image!");

				VkMemoryRequirements memRequirements;
				vkGetImageMemoryRequirements(*m_device, m_image, &memRequirements);

				m_memory = m_device->MemoryPool()->Allocate(memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
				vkBindImageMemory(*m_device, m_image, m_memory->Memory(), m_memory->Offset());
			}

			VulkanStaticTexture::~VulkanStaticTexture() {
				if (m_image != VK_NULL_HANDLE) {
					vkDestroyImage(*m_device, m_image, nullptr);
					m_image = VK_NULL_HANDLE;
				}
			}

			Texture::TextureType VulkanStaticTexture::Type()const {
				return m_textureType;
			}

			Texture::PixelFormat VulkanStaticTexture::ImageFormat()const {
				return m_pixelFormat;
			}

			Texture::Multisampling VulkanStaticTexture::SampleCount()const {
				return m_sampleCount;
			}

			Size3 VulkanStaticTexture::Size()const {
				return m_textureSize;
			}

			uint32_t VulkanStaticTexture::ArraySize()const {
				return m_arraySize;
			}

			uint32_t VulkanStaticTexture::MipLevels()const {
				return m_mipLevels;
			}

			VulkanStaticTexture::operator VkImage()const {
				return m_image;
			}

			VkFormat VulkanStaticTexture::VulkanFormat()const {
				return NativeFormatFromPixelFormat(m_pixelFormat);
			}

			VulkanDevice* VulkanStaticTexture::Device()const {
				return m_device;
			}
		}
	}
}
#pragma warning(default: 26812)

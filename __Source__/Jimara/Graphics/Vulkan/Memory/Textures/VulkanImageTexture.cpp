#include "VulkanImageTexture.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanImageTexture::VulkanImageTexture(
				VulkanDevice* device,
				TextureType type,
				PixelFormat format,
				Size3 size,
				uint32_t arraySize,
				bool generateMipmaps, 
				VkImageUsageFlags usage,
				ImageTexture::AccessFlags accessFlags) 
				: VulkanImage(BaseImageLayout(accessFlags))
				, VulkanTexture(device, type, format, size, arraySize, generateMipmaps, usage, Multisampling::SAMPLE_COUNT_1,
					((accessFlags & ImageTexture::AccessFlags::CPU_READ) == ImageTexture::AccessFlags::NONE) 
						? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
					BaseImageLayout(accessFlags))
				, m_accessFlags(accessFlags)
				, m_pitch([&]()->Size3 {
				if ((accessFlags & ImageTexture::AccessFlags::CPU_READ) == ImageTexture::AccessFlags::NONE)
					return size;
				VkImageSubresource subresource = {};
				{
					subresource.arrayLayer = 0;
					subresource.aspectMask = VulkanImageAspectFlags();
					subresource.mipLevel = 0;
				}
				VkSubresourceLayout layout = {};
				vkGetImageSubresourceLayout(*device, *this, &subresource, &layout);
				const uint32_t bytesPerPixel = static_cast<uint32_t>(BytesPerPixel(ImageFormat()));
				if ((layout.rowPitch % bytesPerPixel) != 0)
					device->Log()->Error("VulkanImageTexture - rowPitch not a multiple of bytesPerPixel! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				if (layout.rowPitch != 0 && (layout.depthPitch % layout.rowPitch) != 0)
					device->Log()->Error("VulkanImageTexture - depthPitch not a multiple of rowPitch! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				if (layout.depthPitch != 0 && (layout.arrayPitch % layout.depthPitch) != 0)
					device->Log()->Error("VulkanImageTexture - arrayPitch not a multiple of depthPitch! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return Size3(
					layout.rowPitch / bytesPerPixel,
					layout.depthPitch / Math::Max(layout.rowPitch, (VkDeviceSize)1u),
					layout.arrayPitch / (layout.depthPitch > 0 ? layout.depthPitch : Math::Max(layout.rowPitch * size.y, (VkDeviceSize)1u)));
					}())
				, m_cpuMappedData(nullptr) {}

			VulkanImageTexture::~VulkanImageTexture() {}

#pragma warning(disable: 26812)
#pragma warning(disable: 26110)
#pragma warning(disable: 26117)
			void* VulkanImageTexture::Map() {
				if (m_cpuMappedData != nullptr) 
					return m_cpuMappedData;

				m_bufferLock.lock();

				if (m_cpuMappedData != nullptr)
					return m_cpuMappedData;

				if ((m_accessFlags & ImageTexture::AccessFlags::CPU_READ) == ImageTexture::AccessFlags::NONE) {
					if (m_stagingBuffer == nullptr) {
						const Size3 size = Size();
						m_stagingBuffer = Object::Instantiate<VulkanArrayBuffer>(Device()
							, VulkanImage::BytesPerPixel(ImageFormat())
							, size.x * size.y * size.z * ArraySize()
							, true
							, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
					}
					m_cpuMappedData = m_stagingBuffer->Map();
				}
				else m_cpuMappedData = Memory()->Map(true);

				return m_cpuMappedData;
			}

			void VulkanImageTexture::Unmap(bool write) {
				if (m_cpuMappedData == nullptr) 
					return;
				if (m_stagingBuffer == nullptr)
					Memory()->Unmap(write);
				else m_stagingBuffer->Unmap(write);
				m_cpuMappedData = nullptr;
				if (write && (m_stagingBuffer != nullptr || MipLevels() > 1u)) {
					auto updateData = [&](VulkanCommandBuffer* commandBuffer) {
						TransitionLayout(commandBuffer, ShaderAccessLayout(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, MipLevels(), 0, ArraySize());

						if (m_stagingBuffer != nullptr) {
							VkBufferImageCopy region = {};
							{
								region.bufferOffset = 0;
								region.bufferRowLength = 0;
								region.bufferImageHeight = 0;

								region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
								region.imageSubresource.mipLevel = 0;
								region.imageSubresource.baseArrayLayer = 0;
								region.imageSubresource.layerCount = ArraySize();

								region.imageOffset = { 0, 0, 0 };
								const Size3 size = Size();
								region.imageExtent = { size.x, size.y, size.z };
							}
							vkCmdCopyBufferToImage(*commandBuffer, *m_stagingBuffer, *this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
							commandBuffer->RecordBufferDependency(m_stagingBuffer);
							m_stagingBuffer = nullptr;
						}

						GenerateMipmaps(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ShaderAccessLayout());
						};
					OneTimeCommandBufferCache()->Execute(updateData);
				}
				m_stagingBuffer = nullptr;
				m_bufferLock.unlock();
			}
#pragma warning(default: 26812)
#pragma warning(default: 26110)
#pragma warning(default: 26117)
		}
	}
}

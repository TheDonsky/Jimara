#include "VulkanCpuWriteOnlyTexture.h"


#pragma warning(disable: 26812)
#pragma warning(disable: 26110)
#pragma warning(disable: 26117)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanCpuWriteOnlyTexture::VulkanCpuWriteOnlyTexture(VulkanDevice* device, TextureType type, PixelFormat format, Size3 size, uint32_t arraySize, bool generateMipmaps)
				: VulkanTexture(device, type, format, size, arraySize, generateMipmaps
					, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
					| VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
					, Multisampling::SAMPLE_COUNT_1, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
				, m_cpuMappedData(nullptr) {}

			VulkanCpuWriteOnlyTexture::~VulkanCpuWriteOnlyTexture() {}


			ImageTexture::CPUAccess VulkanCpuWriteOnlyTexture::HostAccess()const {
				return CPUAccess::CPU_WRITE_ONLY;
			}

			Size3 VulkanCpuWriteOnlyTexture::Pitch()const {
				return Size();
			}

			void* VulkanCpuWriteOnlyTexture::Map() {
				if (m_cpuMappedData != nullptr) return m_cpuMappedData;

				m_bufferLock.lock();

				if (m_stagingBuffer == nullptr) {
					const Size3 size = Size();
					m_stagingBuffer = Object::Instantiate<VulkanArrayBuffer>(Device()
						, VulkanImage::BytesPerPixel(ImageFormat())
						, size.x * size.y * size.z * ArraySize()
						, true
						, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
				}

				m_cpuMappedData = m_stagingBuffer->Map();

				return m_cpuMappedData;
			}

			void VulkanCpuWriteOnlyTexture::Unmap(bool write) {
				if (m_cpuMappedData == nullptr) return;
				m_stagingBuffer->Unmap(write);
				m_cpuMappedData = nullptr;
				if (write) {
					auto updateData = [&](VulkanCommandBuffer* commandBuffer) {
						TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, MipLevels(), 0, ArraySize());

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

						GenerateMipmaps(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
						commandBuffer->RecordBufferDependency(m_stagingBuffer);
						m_stagingBuffer = nullptr;
					};
					OneTimeCommandBufferCache()->Execute(updateData);
				}
				m_stagingBuffer = nullptr;
				m_bufferLock.unlock();
			}
		}
	}
}
#pragma warning(default: 26812)
#pragma warning(default: 26110)
#pragma warning(default: 26117)

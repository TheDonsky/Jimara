#pragma once
#include "VulkanTexture.h"
#include "../Buffers/VulkanArrayBuffer.h"

#pragma warning(disable: 4250)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class JIMARA_API VulkanImageTexture : public virtual VulkanTexture, public virtual ImageTexture {
			public:
				/// <summary>
				/// Shader access layout for given AccessFlags
				/// </summary>
				/// <param name="accessFlags"> Access Flags </param>
				/// <returns> Coresponding ShaderAccessLayout </returns>
				inline static constexpr VkImageLayout BaseImageLayout(ImageTexture::AccessFlags accessFlags) {
					return ((accessFlags & ImageTexture::AccessFlags::SHADER_WRITE) != ImageTexture::AccessFlags::NONE)
						? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				}

				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> Graphics device </param>
				/// <param name="type"> Image type </param>
				/// <param name="format"> Pixel format </param>
				/// <param name="size"> Image size </param>
				/// <param name="arraySize"> Texture array slice count </param>
				/// <param name="generateMipmaps"> If true, mipmaps will be generated </param>
				/// <param name="usage"> Usage flags </param>
				/// <param name="accessFlags"> Device and host access flags </param>
				VulkanImageTexture(
					VulkanDevice* device,
					TextureType type,
					PixelFormat format,
					Size3 size,
					uint32_t arraySize,
					bool generateMipmaps,
					VkImageUsageFlags usage, 
					ImageTexture::AccessFlags accessFlags);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanImageTexture();

				/// <summary> Device access info </summary>
				inline virtual AccessFlags DeviceAccess()const override { return m_accessFlags; }

				/// <summary> Size + padding (in texels) for data index to pixel index translation (always 0 for this one) </summary>
				inline virtual Size3 Pitch()const override { return m_pitch; }

				/// <summary>
				/// Maps texture memory to CPU
				/// Notes:
				///		0. Each Map call should be accompanied by corresponding Unmap() and it's a bad idea to call additional Map()s in between;
				///		1. Depending on the CPUAccess flag used during texture creation(or texture type when CPUAccess does not apply), 
				///		 the actual content of the texture will or will not be present in mapped memory.
				/// </summary>
				/// <returns> Mapped memory </returns>
				virtual void* Map()override;

				/// <summary>
				/// Unmaps memory previously mapped via Map() call
				/// </summary>
				/// <param name="write"> If true, the system will understand that the user modified mapped memory and update the content on GPU </param>
				virtual void Unmap(bool write)override;

			private:
				// Device access info
				const ImageTexture::AccessFlags m_accessFlags;

				// Size with padding
				const Size3 m_pitch;

				// Lock for m_texture and m_stagingBuffer
				std::mutex m_bufferLock;

				// Staging buffer for temporarily holding CPU-mapped data
				Reference<VulkanArrayBuffer> m_stagingBuffer;

				// CPU mapping
				void* m_cpuMappedData;
			};
		}
	}
}
#pragma warning(default: 4250)

#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanDynamicTexture;
		}
	}
}
#include "VulkanTexture.h"
#include "../Buffers/VulkanArrayBuffer.h"
#include "../../../../Core/Synch/SpinLock.h"


#pragma warning(disable: 4250)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// CPU-writable vulkan texture
			/// </summary>
			class JIMARA_API VulkanCpuWriteOnlyTexture : public virtual VulkanTexture, public virtual ImageTexture {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> "Owner" device </param>
				/// <param name="type"> Texture type </param>
				/// <param name="format"> Texture format </param>
				/// <param name="size"> Texture size </param>
				/// <param name="arraySize"> Array slice count </param>
				/// <param name="generateMipmaps"> If true, underlaying images will be auto-generated </param>
				VulkanCpuWriteOnlyTexture(VulkanDevice* device, TextureType type, PixelFormat format, Size3 size, uint32_t arraySize, bool generateMipmaps);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanCpuWriteOnlyTexture();


				/// <summary> CPU access info </summary>
				virtual CPUAccess HostAccess()const override;

				/// <summary> Size + padding (in texels) for data index to pixel index translation (always 0 for this one) </summary>
				virtual Size3 Pitch()const override;

				/// <summary>
				/// Maps texture memory to CPU
				/// Notes:
				///		0. Each Map call should be accompanied by corresponding Unmap() and it's a bad idea to call additional Map()s in between;
				///		1. Depending on the CPUAccess flag used during texture creation(or texture type when CPUAccess does not apply), 
				///		 the actual content of the texture will or will not be present in mapped memory.
				/// </summary>
				/// <returns> Mapped memory </returns>
				virtual void* Map() override;

				/// <summary>
				/// Unmaps memory previously mapped via Map() call
				/// </summary>
				/// <param name="write"> If true, the system will understand that the user modified mapped memory and update the content on GPU </param>
				virtual void Unmap(bool write) override;


			private:
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

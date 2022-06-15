#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanConstantBuffer;
			class VulkanPipelineConstantBuffer;
		}
	}
}
#include "../../Pipeline/VulkanCommandBuffer.h"
#include <optional>
#include <vector>
#include <mutex>


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Vulkan-backed cbuffer
			/// </summary>
			class JIMARA_API VulkanConstantBuffer : public virtual Buffer {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="size"> Number of bytes within the buffer </param>
				VulkanConstantBuffer(size_t size);

				/// <summary> Virtual desrtructor </summary>
				virtual ~VulkanConstantBuffer();

				/// <summary> Size of an individual object/structure within the buffer </summary>
				virtual size_t ObjectSize()const override;

				/// <summary> CPU access info </summary>
				virtual CPUAccess HostAccess()const override;

				/// <summary>
				/// Maps buffer memory to CPU
				/// Notes:
				///		0. Each Map call should be accompanied by corresponding Unmap() and it's a bad idea to call additional Map()s in between;
				///		1. Depending on the CPUAccess flag used during buffer creation(or buffer type when CPUAccess does not apply), 
				///		 the actual content of the buffer will or will not be present in mapped memory.
				/// </summary>
				/// <returns> Mapped memory </returns>
				virtual void* Map() override;

				/// <summary>
				/// Unmaps memory previously mapped via Map() call
				/// </summary>
				/// <param name="write"> If true, the system will understand that the user modified mapped memory and update the content on GPU </param>
				virtual void Unmap(bool write) override;


			private:
				// Buffer size
				const size_t m_size;

				// Data
				uint8_t* m_data;

				// Data
				uint8_t* m_mappedData;

				// Data map lock
				std::mutex m_lock;

				// Number of map & unmap cycles so far
				std::atomic<uint64_t> m_revision;

				// VulkanPipelineConstantBuffer needs to access data
				friend class VulkanPipelineConstantBuffer;
			};


			/// <summary>
			/// GPU-side constant buffer copy, managed by pipelines
			/// </summary>
			class JIMARA_API VulkanPipelineConstantBuffer : public virtual Object {
			public:
				VulkanPipelineConstantBuffer(VulkanDevice* device, VulkanConstantBuffer* buffer, size_t commandBufferCount);

				/// <summary> Virtual desrtructor </summary>
				virtual ~VulkanPipelineConstantBuffer();

				/// <summary> Target buffer </summary>
				VulkanConstantBuffer* TargetBuffer()const;

				/// <summary>
				/// Gets appropriate buffer based on the recorder
				/// </summary>
				/// <param name="commandBufferIndex"> Command buffer index </param>
				/// <returns> Attachment buffer and offset </returns>
				std::pair<VkBuffer, VkDeviceSize> GetBuffer(size_t commandBufferIndex);



			private:
				// "Owner" device
				const Reference<VulkanDevice> m_device;

				// Data buffer
				const Reference<VulkanConstantBuffer> m_constantBuffer;

				// Actual buffer that is read by the shaders
				struct Attachment {
					// Memory offset
					VkDeviceSize memoryOffset;

					// Last revision
					std::optional<uint64_t> resvison;

					// Constructor
					inline Attachment(VkDeviceSize off = 0) : memoryOffset(off) {}
				};

				// Buffer
				VkBuffer m_buffer;

				// Syb-buffers
				std::vector<Attachment> m_buffers;

				// Buffer memory allocation
				Reference<VulkanMemoryAllocation> m_memory;
			};
		}
	}
}

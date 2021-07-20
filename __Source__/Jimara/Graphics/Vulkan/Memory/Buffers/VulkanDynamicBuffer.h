#pragma once
#include "VulkanStaticBuffer.h"
#include "../VulkanDynamicDataUpdater.h"
#include "../../../../Core/Synch/SpinLock.h"
#include <mutex>


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Vulkan buffer that resides on GPU memory and maps to a staging buffer for writes
			/// Note: CPU_READ_WRITE is implemented, but I would not call it fully functional just yet, since you can still map the memory while in use by GPU <_TODO_>
			/// </summary>
			class VulkanDynamicBuffer : public virtual VulkanArrayBuffer {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> "Owner" device </param>
				/// <param name="objectSize"> Size of an individual object within the buffer </param>
				/// <param name="objectCount"> Count of objects within the buffer </param>
				VulkanDynamicBuffer(VulkanDevice* device, size_t objectSize, size_t objectCount);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanDynamicBuffer();

				/// <summary> Size of an individual object/structure within the buffer </summary>
				virtual size_t ObjectSize()const override;

				/// <summary> Number of objects within the buffer </summary>
				virtual size_t ObjectCount()const override;

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

				/// <summary>
				/// Access data buffer
				/// </summary>
				/// <param name="commandBuffer"> Command buffer that relies on the resource </param>
				/// <returns> Reference to the data buffer </returns>
				virtual Reference<VulkanStaticBuffer> GetStaticHandle(VulkanCommandBuffer* commandBuffer)override;


			private:
				// "Owner" device
				const Reference<VulkanDevice> m_device;

				// Size of an individual object within the buffer
				const size_t m_objectSize;

				// Count of objects within the buffer
				const size_t m_objectCount;

				// Lock for m_dataBuffer and m_stagingBuffer
				std::mutex m_bufferLock;

				// Lock for m_dataBuffer reference
				SpinLock m_dataBufferSpin;

				// GPU-side data buffer
				Reference<VulkanStaticBuffer> m_dataBuffer;

				// CPU-Mapped memory buffer
				Reference<VulkanStaticBuffer> m_stagingBuffer;

				// CPU-Mapped data
				void* m_cpuMappedData;

				// Data updater
				VulkanDynamicDataUpdater m_updater;

				// Data update function
				void UpdateData(VulkanCommandBuffer* commandBuffer);
			};
		}
	}
}

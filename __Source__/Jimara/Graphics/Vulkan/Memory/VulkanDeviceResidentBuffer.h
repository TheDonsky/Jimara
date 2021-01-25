#pragma once
#include "VulkanBuffer.h"
#include "../Rendering/VulkanRenderEngine.h"
#include <mutex>


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanDeviceResidentBuffer : public virtual ArrayBuffer {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> "Owner" device </param>
				/// <param name="objectSize"> Size of an individual object within the buffer </param>
				/// <param name="objectCount"> Count of objects within the buffer </param>
				/// <param name="cpuAccess"> CPU access flags </param>
				VulkanDeviceResidentBuffer(VulkanDevice* device, size_t objectSize, size_t objectCount, CPUAccess cpuAccess);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanDeviceResidentBuffer();

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
				/// <param name="commandRecorder"> Command recorder for flushing any modifications if necessary </param>
				/// <returns> Reference to the data buffer </returns>
				Reference<VulkanBuffer> GetDataBuffer(VulkanRenderEngine::CommandRecorder* commandRecorder);


			private:
				// "Owner" device
				const Reference<VulkanDevice> m_device;

				// Size of an individual object within the buffer
				const size_t m_objectSize;

				// Count of objects within the buffer
				const size_t m_objectCount;

				// CPU access flags
				const CPUAccess m_cpuAccess;

				// Lock for m_dataBuffer and m_stagingBuffer
				std::mutex m_bufferLock;

				// GPU-side data buffer
				Reference<VulkanBuffer> m_dataBuffer;

				// CPU-Mapped memory buffer
				Reference<VulkanBuffer> m_stagingBuffer;

				// CPU-Mapped data
				void* m_cpuMappedData;
			};
		}
	}
}

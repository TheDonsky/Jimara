#pragma once
#include "../Pipeline/Commands/VulkanCommandBuffer.h"
#include "../../../Core/Function.h"
#include <queue>
#include <mutex>

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Helper for some dynamic storage types that need to internally execute PrimaryCommandBuffer::BeginRecording asynchronously. This is a handy tool to keep some command buffers alive
			/// </summary>
			class JIMARA_API VulkanOneTimeCommandBufferCache {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> Vulkan device </param>
				VulkanOneTimeCommandBufferCache(VulkanDevice* device);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanOneTimeCommandBufferCache();

				/// <summary> Clears all command buffer records </summary>
				void Clear();

				/// <summary>
				/// Executes VulkanDevice::SubmitOneTimeCommandBuffer and keeps it alive till the moment the GPU is done with the buffer or VulkanDynamicDataUpdater goes out of scope
				/// <para/> Make sure the recorded commands do not create any circular references!!
				/// </summary>
				/// <param name="recordCommands"> Invoked in-between PrimaryCommandBuffer::BeginRecording and PrimaryCommandBuffer::EndRecording to record commands </param>
				void Execute(Callback<VulkanPrimaryCommandBuffer*> recordCommands);

				/// <summary>
				/// Executes VulkanDevice::SubmitOneTimeCommandBuffer and keeps it alive till the moment the GPU is done with the buffer or VulkanDynamicDataUpdater goes out of scope
				/// <para/> Make sure the recorded commands do not create any circular references!!
				/// </summary>
				/// <typeparam name="RecordCommandsFn"> Anything, that can be invoked with VulkanPrimaryCommandBuffer* as an argument </typeparam>
				/// <param name="recordCommands"> Invoked in-between PrimaryCommandBuffer::BeginRecording and PrimaryCommandBuffer::EndRecording to record commands </param>
				template<typename RecordCommandsFn>
				inline void Execute(const RecordCommandsFn& recordCommands) {
					Execute(Callback<VulkanPrimaryCommandBuffer*>::FromCall(&recordCommands));
				}


			private:
				// Device
				const Reference<VulkanDevice> m_device;

				// Device-specific object that occasionally cleans m_updateBuffers from another thread
				const Reference<Object> m_drain;

				// Object, that holds a weak reference to the VulkanOneTimeCommandBufferCache and can be added to the drain queue
				const Reference<Object> m_drainer;

				// Submitted command buffers (We need these to avoid CPU-GPU synchronisations mid render)
				std::queue<VulkanDevice::OneTimeCommandBufferInfo> m_updateBuffers;

				// Some private functionality resides here
				struct Helpers;
			};
		}
	}
}

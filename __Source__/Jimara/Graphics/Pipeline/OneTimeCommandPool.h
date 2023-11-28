#pragma once
#include "../GraphicsDevice.h"
#include <thread>
#include <condition_variable>
#include <queue>
#include <stack>

namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Occasionally we need to perform some one-off graphics operations.
		/// Creating command pools and command buffers each time and managing their lifecycle can become cumbersome
		/// and this is sort-of a solution for that:
		/// You just obtain and keep a shared instance of a OneTimeCommandPool for given device and use OneTimeCommandPool::Buffer 
		/// whenever you need to submit anything to the graphics queue.
		/// </summary>
		class JIMARA_API OneTimeCommandPool : public virtual Object {
		public:
			/// <summary> 
			/// In order to use OneTimeCommandPool, one should create a OneTimeCommandPool::Buffer and record commands;
			/// Once the Buffer goes out of scope, the underlying CommandBuffer will be queued on the graphics queue 
			/// </summary>
			class JIMARA_API Buffer;

			/// <summary>
			/// Gets shared OneTimeCommandPool for given device
			/// </summary>
			/// <param name="device"> Device </param>
			/// <returns> Shared OneTimeCommandPool instance </returns>
			static Reference<OneTimeCommandPool> GetFor(GraphicsDevice* device);

			/// <summary> Virtual destructor </summary>
			virtual ~OneTimeCommandPool();

			/// <summary> Graphics device </summary>
			inline GraphicsDevice* Device()const { return m_device; }

		private:
			// Graphics Device
			const Reference<GraphicsDevice> m_device;

			// Command buffer instance
			struct CommandBufferInstance : public virtual Object {
				const Reference<CommandPool> pool;
				const Reference<PrimaryCommandBuffer> buffer;
				inline CommandBufferInstance(CommandPool* p, PrimaryCommandBuffer* b) 
					: pool(p), buffer(b) {
					assert(p != nullptr);
					assert(b != nullptr);
				}
				inline virtual ~CommandBufferInstance() {}
			};

			// Command buffers that are available
			std::stack<Reference<CommandBufferInstance>> m_freeCommandBuffers;

			// Command buffers that are currently running
			std::queue<Reference<CommandBufferInstance>> m_runningCommandBuffers;

			// Lock for managing m_freeCommandBuffers and m_runningCommandBuffers
			std::mutex m_bufferAllocationLock;

			// Triggered whenever there's an addition in m_runningCommandBuffers or the pool dies
			std::condition_variable m_runningCommandBufferAdded;

			// Thread for 'joining' rinning command buffers
			std::thread m_waitThread;

			// Becomes true at the start of the destructor
			volatile bool m_dead = false;

			// Constructor is private:
			OneTimeCommandPool(GraphicsDevice* device);

			// Copy not allowed:
			OneTimeCommandPool(const OneTimeCommandPool&) = delete;
			OneTimeCommandPool(OneTimeCommandPool&&) = delete;
			OneTimeCommandPool& operator=(const OneTimeCommandPool&) = delete;
			OneTimeCommandPool& operator=(OneTimeCommandPool&&) = delete;
		};


		/// <summary> 
		/// In order to use OneTimeCommandPool, one should create a OneTimeCommandPool::Buffer and record commands;
		/// Once the Buffer goes out of scope, the underlying CommandBuffer will be queued on the graphics queue.
		/// </summary>
		class JIMARA_API OneTimeCommandPool::Buffer final {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="pool"> OneTimeCommandPool </param>
			Buffer(OneTimeCommandPool* pool);

			/// <summary>
			/// Destructor (submits command buffer on the queue without waiting)
			/// </summary>
			~Buffer();

			/// <summary> Underlying command buffer </summary>
			inline operator CommandBuffer* ()const { return m_buffer == nullptr ? nullptr : m_buffer->buffer; }

			/// <summary> True, if command buffer exists </summary>
			inline operator bool()const { return operator Jimara::Graphics::CommandBuffer * () != nullptr; }

			/// <summary> 
			/// Submits command buffer on the queue without waiting (this is the default destructor behaviour)
			/// <para/> Note that underlying command buffer reference will be lost.
			/// </summary>
			void SubmitAsynch();

			/// <summary>
			/// Submits command buffer on the queue and waits for the execution to finish
			/// <para/> Note that underlying command buffer reference will be lost.
			/// </summary>
			void SubmitAndWait();

		private:
			// One time command pool:
			const Reference<OneTimeCommandPool> m_pool;

			// Command buffer:
			Reference<CommandBufferInstance> m_buffer;

			// Lock for submitions:
			SpinLock m_submissionLock;

			// Copy not allowed:
			inline Buffer(const Buffer&) = delete;
			inline Buffer(Buffer&&) = delete;
			inline Buffer& operator=(const Buffer&) = delete;
			inline Buffer& operator=(Buffer&&) = delete;
		};
	}
}

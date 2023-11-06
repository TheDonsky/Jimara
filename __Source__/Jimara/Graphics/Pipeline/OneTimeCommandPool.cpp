#include "OneTimeCommandPool.h"


namespace Jimara {
	namespace Graphics {
		Reference<OneTimeCommandPool> OneTimeCommandPool::GetFor(GraphicsDevice* device) {
			if (device == nullptr)
				return nullptr;

#pragma warning (disable: 4250)
			class CachedPool
				: public virtual OneTimeCommandPool
				, public virtual ObjectCache<Reference<const Object>>::StoredObject {
			public:
				inline CachedPool(GraphicsDevice* dev) : OneTimeCommandPool(dev) {}
				inline virtual ~CachedPool() {}
			};
#pragma warning (default: 4250)

			class PoolCache : public virtual ObjectCache<Reference<const Object>> {
			public:
				inline Reference<OneTimeCommandPool> Get(GraphicsDevice* dev) {
					return GetCachedOrCreate(dev, false, [&]() { return Object::Instantiate<CachedPool>(dev); });
				}
			};

			static PoolCache cache;
			return cache.Get(device);
		}

		OneTimeCommandPool::OneTimeCommandPool(GraphicsDevice* device) : m_device(device) {
			assert(m_device != nullptr);
			
			static const auto threadFn = [](OneTimeCommandPool* self) {
				while (true) {
					// Obtain buffer from top:
					const Reference<CommandBufferInstance> buffer = [&]() -> Reference<CommandBufferInstance> {
						std::unique_lock<std::mutex> lock(self->m_bufferAllocationLock);
						while (self->m_runningCommandBuffers.empty()) {
							if (self->m_dead) return nullptr;
							else self->m_runningCommandBufferAdded.wait(lock);
						}
						Reference<CommandBufferInstance> buff = self->m_runningCommandBuffers.front();
						self->m_runningCommandBuffers.pop();
						assert(buff != nullptr);
						return buff;
					}();
					
					// If buffer is null, that means the pool is dead:
					if (buffer == nullptr)
						return;
					
					// Wait for execution:
					{
						buffer->buffer->Wait();
						buffer->buffer->Reset();
					}

					// Add buffer (back) to the free buffer stack:
					{
						std::unique_lock<std::mutex> lock(self->m_bufferAllocationLock);
						self->m_freeCommandBuffers.push(buffer);
					}
				}
			};

			m_waitThread = std::thread(threadFn, this);
		}

		OneTimeCommandPool::~OneTimeCommandPool() {
			{
				std::unique_lock<std::mutex> lock(m_bufferAllocationLock);
				m_dead = true;
				m_runningCommandBufferAdded.notify_one();
			}
			m_waitThread.join();
			assert(m_runningCommandBuffers.empty());
		}


		OneTimeCommandPool::Buffer::Buffer(OneTimeCommandPool* pool) : m_pool(pool) {
			// If there's no pool, there is no point in doing anything more:
			if (m_pool == nullptr)
				return;

			// Try to obtain free command buffer:
			m_buffer = [&]() -> Reference<CommandBufferInstance> {
				std::unique_lock<std::mutex> lock(m_pool->m_bufferAllocationLock);
				if (m_pool->m_freeCommandBuffers.empty())
					return nullptr;
				const Reference<CommandBufferInstance> top = m_pool->m_freeCommandBuffers.top();
				m_pool->m_freeCommandBuffers.pop();
				assert(top != nullptr);
				return top;
			}();

			// If there was no free command buffer, we should create a new one:
			if (m_buffer == nullptr) {
				const Reference<CommandPool> pool = m_pool->Device()->GraphicsQueue()->CreateCommandPool();
				if (pool == nullptr) {
					m_pool->Device()->Log()->Error("OneTimeCommandPool::Buffer::Buffer - Failed to create command pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return;
				}
				const Reference<PrimaryCommandBuffer> buffer = pool->CreatePrimaryCommandBuffer();
				if (buffer == nullptr) {
					m_pool->Device()->Log()->Error("OneTimeCommandPool::Buffer::Buffer - Failed to create command buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return;
				}
				m_buffer = Object::Instantiate<CommandBufferInstance>(pool, buffer);
			}

			// Begin recording:
			assert(m_buffer != nullptr);
			m_buffer->buffer->BeginRecording();
		}

		OneTimeCommandPool::Buffer::~Buffer() {
			SubmitAsynch();
		}

		void OneTimeCommandPool::Buffer::SubmitAsynch() {
			std::unique_lock<SpinLock> submitLock(m_submissionLock);

			// If there was no command buffer, there's no need to go further:
			if (m_buffer == nullptr)
				return;

			// Submit buffer:
			{
				m_buffer->buffer->EndRecording();
				m_pool->Device()->GraphicsQueue()->ExecuteCommandBuffer(m_buffer->buffer);
			}

			// Schedule on wait thread:
			{
				std::unique_lock<std::mutex> lock(m_pool->m_bufferAllocationLock);
				m_pool->m_runningCommandBuffers.push(m_buffer);
				m_pool->m_runningCommandBufferAdded.notify_one();
			}

			// Remove buffer:
			m_buffer = nullptr;
		}

		void OneTimeCommandPool::Buffer::SubmitAndWait() {
			std::unique_lock<SpinLock> submitLock(m_submissionLock);

			// If there was no command buffer, there's no need to go further:
			if (m_buffer == nullptr)
				return;

			// Submit buffer and wait for it:
			{
				m_buffer->buffer->EndRecording();
				m_pool->Device()->GraphicsQueue()->ExecuteCommandBuffer(m_buffer->buffer);
				m_buffer->buffer->Wait();
			}

			// Add (back) to the free buffers:
			{
				std::unique_lock<std::mutex> lock(m_pool->m_bufferAllocationLock);
				m_pool->m_freeCommandBuffers.push(m_buffer);
			}

			// Remove buffer:
			m_buffer = nullptr;
		}
	}
}

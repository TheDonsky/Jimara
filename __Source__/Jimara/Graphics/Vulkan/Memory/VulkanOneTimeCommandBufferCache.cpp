#include "VulkanOneTimeCommandBufferCache.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			struct VulkanOneTimeCommandBufferCache::Helpers {
				inline static void DrainBuffers(VulkanOneTimeCommandBufferCache* cache) {
					while (!cache->m_updateBuffers.empty()) {
						const auto& info = cache->m_updateBuffers.front();
						if (info.timeline->Count() < info.timelineValue) {
							dynamic_cast<Drain*>(cache->m_drain.operator->())->Enqueue(
								dynamic_cast<Drainer*>(cache->m_drainer.operator->()));
							break;
						}
						else cache->m_updateBuffers.pop();
					}
				}
				
				struct Drainer : public virtual Object {
					std::mutex lock;
					VulkanOneTimeCommandBufferCache* owner = nullptr;

					inline void Drain() {
						std::unique_lock<std::mutex> drainLock(lock);
						if (owner != nullptr)
							DrainBuffers(owner);
					}
				};
				
				class Drain : public virtual ObjectCache<Reference<const Object>>::StoredObject {
				private:
					std::vector<Reference<Drainer>> m_buffers[2];
					SpinLock m_backBufferLock;
					std::atomic<size_t> m_backBuffer = 0;
					std::thread m_updateThread;
					std::atomic<bool> m_dead = false;

				public:
					inline Drain() {
						void (*update)(Drain*) = [](Drain* self) {
							while (!self->m_dead.load()) {
								std::vector<Reference<Drainer>>* frontBuffer;
								{
									std::unique_lock<SpinLock> lock(self->m_backBufferLock);
									frontBuffer = self->m_buffers + self->m_backBuffer.load();
									self->m_backBuffer = (self->m_backBuffer.load() + 1) & 1;
								}
								for (size_t i = 0; i < frontBuffer->size(); i++)
									frontBuffer->operator[](i)->Drain();
								frontBuffer->clear();
								std::this_thread::sleep_for(std::chrono::microseconds(8));
							}
						};
						m_updateThread = std::thread(update, this);
					}

					inline virtual ~Drain() {
						m_dead = true;
						m_updateThread.join();
					}

					inline void Enqueue(Drainer* drainer) {
						std::unique_lock<SpinLock> lock(m_backBufferLock);
						m_buffers[m_backBuffer.load()].push_back(drainer);
					}
				};

				class DrainCache : public virtual ObjectCache<Reference<const Object>> {
				public:
					inline static const Reference<Drain> GetDrain(VkDeviceHandle* device) {
						static DrainCache cache;
						return cache.GetCachedOrCreate(device, Object::Instantiate<Drain>);
					}
				};
			};

			VulkanOneTimeCommandBufferCache::VulkanOneTimeCommandBufferCache(VulkanDevice* device)
				: m_device(device)
				, m_drain(Helpers::DrainCache::GetDrain(device->operator Jimara::Graphics::Vulkan::VkDeviceHandle *()))
				, m_drainer(Object::Instantiate<Helpers::Drainer>()) {
				Helpers::Drainer* drainer = dynamic_cast<Helpers::Drainer*>(m_drainer.operator->());
				drainer->owner = this;
			}

			VulkanOneTimeCommandBufferCache::~VulkanOneTimeCommandBufferCache() {
				Helpers::Drainer* drainer = dynamic_cast<Helpers::Drainer*>(m_drainer.operator->());
				std::unique_lock<std::mutex> lock(drainer->lock);
				drainer->owner = nullptr;
			}

			void VulkanOneTimeCommandBufferCache::Clear() {
				Helpers::Drainer* drainer = dynamic_cast<Helpers::Drainer*>(m_drainer.operator->());
				std::unique_lock<std::mutex> lock(drainer->lock);
				while (!m_updateBuffers.empty()) {
					m_updateBuffers.front().commandBuffer->Wait();
					m_updateBuffers.pop();
				}
			}

			void VulkanOneTimeCommandBufferCache::Execute(Callback<VulkanPrimaryCommandBuffer*> recordCommands) {
				Helpers::Drainer* drainer = dynamic_cast<Helpers::Drainer*>(m_drainer.operator->());
				std::unique_lock<std::mutex> lock(drainer->lock);
				m_updateBuffers.push(m_device->SubmitOneTimeCommandBuffer(recordCommands));
				Helpers::DrainBuffers(this);
			}
		}
	}
}

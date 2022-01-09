#include "GraphicsContext.h"
#include "../../../__Generated__/JIMARA_BUILT_IN_LIGHT_IDENTIFIERS.h"


namespace Jimara {
#ifndef USE_REFACTORED_SCENE
namespace Refactor_TMP_Namespace {
#endif
	bool Scene::GraphicsContext::ConfigurationSettings::GetLightTypeId(const std::string& lightTypeName, uint32_t& lightTypeId)const {
		std::unordered_map<std::string, uint32_t>::const_iterator it = m_lightTypeIds.find(lightTypeName);
		if (it == m_lightTypeIds.end()) return false;
		else {
			lightTypeId = it->second;
			return true;
		}
	}

	Scene::GraphicsContext::ConfigurationSettings::ConfigurationSettings(const GraphicsConstants& constants) 
		: m_maxInFlightCommandBuffers(constants.maxInFlightCommandBuffers)
		, m_shaderLoader(constants.shaderLoader)
		, m_lightTypeIds(*constants.lightSettings.lightTypeIds)
		, m_perLightDataSize(constants.lightSettings.perLightDataSize) {}

	namespace {
		typedef std::pair<Reference<Object>, Callback<>> WorkerCleanupCall;

		class WorkerCleanupList {
		private:
			SpinLock* const m_lock;
			std::vector<WorkerCleanupCall>* const m_list;

		public:
			inline WorkerCleanupList(SpinLock* lock, std::vector<WorkerCleanupCall>* list)
				: m_lock(lock), m_list(list) {}

			inline void Push(const WorkerCleanupCall& call) {
				std::unique_lock<SpinLock> lock(*m_lock);
				m_list->push_back(call);
			}

			inline void Cleanup() {
				std::unique_lock<SpinLock> lock(*m_lock);
				for (size_t i = 0; i < m_list->size(); i++)
					m_list->operator[](i).second();
				m_list->clear();
			}
		};

		typedef std::pair<Reference<Object>, Reference<Graphics::PrimaryCommandBuffer>> PoolAndBuffer;
		typedef std::pair<PoolAndBuffer, Callback<Graphics::PrimaryCommandBuffer*>> CommandBufferReleaseCall;
		typedef std::vector<CommandBufferReleaseCall> CommandBufferReleaseList;

		class WorkerCommandPool : public virtual ObjectCache<Reference<Object>>::StoredObject {
		private:
			const Reference<Graphics::GraphicsDevice> m_device;
			const Reference<Graphics::CommandPool> m_commandPool;
			std::vector<Reference<Graphics::PrimaryCommandBuffer>> m_freeBuffers;
			Reference<Graphics::PrimaryCommandBuffer> m_currentCommandBuffer;
			SpinLock m_lock;

		public:
			inline WorkerCommandPool(Scene::GraphicsContext* context) 
				: m_device(context->Device())
				, m_commandPool(context->Device()->GraphicsQueue()->CreateCommandPool()) {
				if (m_commandPool == nullptr)
					m_device->Log()->Fatal(
						"Scene::GraphicsContext::GetWorkerThreadCommandBuffer - Failed to create command pool! [File: '", __FILE__, "'; Line:", __LINE__, "]");
			}

			template<typename AddReleaseBuffer>
			inline Reference<Graphics::CommandBuffer> GetCommandBuffer(WorkerCleanupList& cleanup, const AddReleaseBuffer& addReleaseBuffer) {
				std::unique_lock<SpinLock> lock(m_lock);
				
				// If the current iteration already has a command buffer, return it:
				if (m_currentCommandBuffer != nullptr) 
					return m_currentCommandBuffer;
				
				// Allocate new command buffers if needed:
				if (m_freeBuffers.size() <= 0) {
					Reference<Graphics::CommandBuffer> newBuffer = m_commandPool->CreatePrimaryCommandBuffer();
					if (newBuffer == nullptr) {
						m_device->Log()->Fatal(
							"Scene::GraphicsContext::GetWorkerThreadCommandBuffer - Failed to create a command buffer! [File: '", __FILE__, "'; Line:", __LINE__, "]");
						return nullptr;
					}
					else m_freeBuffers.push_back(newBuffer);
				}

				// Set current command buffer and start recording:
				{
					m_currentCommandBuffer = m_freeBuffers.back();
					m_freeBuffers.pop_back();
					m_currentCommandBuffer->BeginRecording();
				}

				// Add cleanup routine:
				{
					void(*cleanupCall)(WorkerCommandPool*) = [](WorkerCommandPool* pool) {
						assert(pool != nullptr);
						std::unique_lock<SpinLock> lock(pool->m_lock);
						if (pool->m_currentCommandBuffer == nullptr) return;
						pool->m_currentCommandBuffer->EndRecording();
						pool->m_device->GraphicsQueue()->ExecuteCommandBuffer(pool->m_currentCommandBuffer);
						pool->m_currentCommandBuffer = nullptr;
					};
					cleanup.Push(WorkerCleanupCall(this, Callback<>(cleanupCall, this)));
				}

				// Add recycling routine:
				{
					void(*releaseRoutine)(WorkerCommandPool*, Graphics::PrimaryCommandBuffer*) = [](WorkerCommandPool* pool, Graphics::PrimaryCommandBuffer* buffer) {
						if (buffer == nullptr) return;
						assert(pool != nullptr);
						std::unique_lock<SpinLock> lock(pool->m_lock);
						buffer->Wait();
						buffer->Reset();
						pool->m_freeBuffers.push_back(buffer);
					};
					addReleaseBuffer(CommandBufferReleaseCall(PoolAndBuffer(this, m_currentCommandBuffer),
						Callback<Graphics::PrimaryCommandBuffer*>(releaseRoutine, this)));
				}

				// Safetly return the command buffer:
				{
					Reference<Graphics::CommandBuffer> result = m_currentCommandBuffer;
					return result;
				}
			}
		};

		class WorkerCommandPoolCache : public virtual ObjectCache<Reference<Object>> {
		private:
			Reference<Scene::GraphicsContext> m_lastQueryContext;
			Reference<WorkerCommandPool> m_lastQueryPool;
			SpinLock m_lock;

		public:
			inline Reference<WorkerCommandPool> GetFor(Scene::GraphicsContext* context, WorkerCleanupList* cleanup) {
				std::unique_lock<SpinLock> lock(m_lock);

				// Check if we are just repeating the last call:
				if (m_lastQueryContext != nullptr && m_lastQueryContext == context)
					return m_lastQueryPool;

				// If the context is null, we just release the reference (ei it's a cleanup):
				m_lastQueryContext = context;
				if (context == nullptr) {
					m_lastQueryPool = nullptr;
					return nullptr;
				}

				// Record cleanup call:
				if (cleanup != nullptr) {
					void(*cleanupCall)(WorkerCommandPoolCache*) = [](WorkerCommandPoolCache* cache) {
						cache->GetFor(nullptr, nullptr);
					};
					cleanup->Push(WorkerCleanupCall(this, Callback<>(cleanupCall, this)));
				}

				// Return the pool:
				m_lastQueryPool = GetCachedOrCreate(context, false, [&]() -> Reference<WorkerCommandPool> {
					return Object::Instantiate<WorkerCommandPool>(context);
					});
				return m_lastQueryPool;
			}

			inline static Reference<WorkerCommandPoolCache> ForThisThread() {
				static thread_local Reference<WorkerCommandPoolCache> cache = Object::Instantiate<WorkerCommandPoolCache>();
				return cache;
			}
		};
	}

	Graphics::Pipeline::CommandBufferInfo Scene::GraphicsContext::GetWorkerThreadCommandBuffer() {
		assert(this != nullptr);
		
		// Make sure we have a right to get the command buffer:
		Reference<Data> data = m_data;
		if (data == nullptr) {
			Device()->Log()->Error("Scene::GraphicsContext::GetWorkerThreadCommandBuffer - Scene out of scope!");
			return Graphics::Pipeline::CommandBufferInfo(nullptr, 0);
		}
		else if (!m_frameData.canGetWorkerCommandBuffer) {
			Device()->Log()->Error("Scene::GraphicsContext::GetWorkerThreadCommandBuffer - Not a valid context to get a command buffer from!");
			return Graphics::Pipeline::CommandBufferInfo(nullptr, 0);
		}

		// Get thread_local cache and WorkerCommandPoolCache:
		Reference<WorkerCommandPoolCache> cache = WorkerCommandPoolCache::ForThisThread();
		if (cache == nullptr) {
			Device()->Log()->Fatal(
				"Scene::GraphicsContext::GetWorkerThreadCommandBuffer - Failed to retrieve/create command pool cache! [File: '", __FILE__, "'; Line:", __LINE__, "]");
			return Graphics::Pipeline::CommandBufferInfo(nullptr, 0);
		}

		// Get the worker command pool:
		WorkerCleanupList workerCleanupList(&data->workerCleanupLock, &data->workerCleanupJobs);
		Reference<WorkerCommandPool> commandPool = cache->GetFor(this, &workerCleanupList);
		if (commandPool == nullptr) {
			Device()->Log()->Fatal(
				"Scene::GraphicsContext::GetWorkerThreadCommandBuffer - Failed to retrieve/create command pool! [File: '", __FILE__, "'; Line:", __LINE__, "]");
			return Graphics::Pipeline::CommandBufferInfo(nullptr, 0);
		}

		// Get command buffer:
		Reference<Graphics::CommandBuffer> commandBuffer = commandPool->GetCommandBuffer(workerCleanupList, [&](const CommandBufferReleaseCall& releaseCall) {
			std::unique_lock<SpinLock> lock(data->workerCleanupLock);
			data->inFlightBufferCleanupJobs[m_frameData.inFlightWorkerCommandBufferId].push_back(releaseCall);
			});
		return Graphics::Pipeline::CommandBufferInfo(commandBuffer, m_frameData.inFlightWorkerCommandBufferId);
	}

	namespace {
		class EmptyEvent : public virtual Event<> {
		public:
			inline virtual void operator+=(Callback<>) final override {}
			inline virtual void operator-=(Callback<> callback) final override {}
			inline static EmptyEvent& Instance() {
				static EmptyEvent evt;
				return evt;
			}
		};

		class EmptyJobSet : public virtual JobSystem::JobSet {
		public:
			inline virtual void Add(JobSystem::Job*) final override {}
			inline virtual void Remove(JobSystem::Job* job) final override {}
			inline static EmptyJobSet& Instance() {
				static EmptyJobSet set;
				return set;
			}
		};
	}

	Event<>& Scene::GraphicsContext::PreGraphicsSynch() {
		Reference<Data> data = m_data;
		if (data != nullptr) return EmptyEvent::Instance();
		else return data->onPreSynch;
	}

	JobSystem::JobSet& Scene::GraphicsContext::SynchPointJobs() {
		Reference<Data> data = m_data;
		if (data != nullptr) return data->synchJob.Jobs();
		else return EmptyJobSet::Instance();
	}

	Event<>& Scene::GraphicsContext::OnGraphicsSynch() {
		Reference<Data> data = m_data;
		if (data != nullptr) return EmptyEvent::Instance();
		else return data->onSynch;
	}

	JobSystem::JobSet& Scene::GraphicsContext::RenderJobs() {
		Reference<Data> data = m_data;
		if (data != nullptr) return data->renderJob;
		else return EmptyJobSet::Instance();
	}

	void Scene::GraphicsContext::RenderStack::AddRenderer(Renderer* renderer) {
		if (renderer == nullptr) return;
		Reference<GraphicsContext> context = m_context;
		if (context == nullptr) return;
		Reference<Data> data = context->m_data;
		if (data == nullptr) return;
		std::unique_lock<std::mutex> rendererLock(data->rendererLock);
		data->rendererSet.ScheduleAdd(renderer);
	}

	void Scene::GraphicsContext::RenderStack::RemoveRenderer(Renderer* renderer) {
		if (renderer == nullptr) return;
		Reference<GraphicsContext> context = m_context;
		if (context == nullptr) return;
		Reference<Data> data = context->m_data;
		if (data == nullptr) return;
		std::unique_lock<std::mutex> rendererLock(data->rendererLock);
		data->rendererSet.ScheduleRemove(renderer);
	}

	Reference<Graphics::TextureView> Scene::GraphicsContext::RenderStack::TargetTexture()const {
		std::unique_lock<SpinLock> lock(m_currentTargetTextureLock);
		Reference<Graphics::TextureView> view = m_currentTargetTexture;
		return view;
	}

	void Scene::GraphicsContext::RenderStack::SetTargetTexture(Graphics::TextureView* targetTexture) {
		std::unique_lock<SpinLock> lock(m_currentTargetTextureLock);
		m_currentTargetTexture = targetTexture;
	}

	Event<>& Scene::GraphicsContext::OnRenderFinished() {
		Reference<Data> data = m_data;
		if (data != nullptr) return EmptyEvent::Instance();
		else return data->onRenderFinished;
	}



	inline Scene::GraphicsContext::GraphicsContext(const Scene::GraphicsConstants& constants)
		: m_device(constants.graphicsDevice)
		, m_configuration(constants)
		, m_rendererStack(this) {}

	void Scene::GraphicsContext::Sync() {
		Reference<Data> data = m_data;
		if (data == nullptr) return;
		
		// Increment in-flight buffer id:
		{
			std::unique_lock<SpinLock> lock(data->workerCleanupLock);
			m_frameData.inFlightWorkerCommandBufferId = (m_frameData.inFlightWorkerCommandBufferId + 1) % Configuration().MaxInFlightCommandBufferCount();
			if (data->inFlightBufferCleanupJobs.size() <= m_frameData.inFlightWorkerCommandBufferId)
				data->inFlightBufferCleanupJobs.resize(Configuration().MaxInFlightCommandBufferCount());
			else {
				CommandBufferReleaseList& list = data->inFlightBufferCleanupJobs[m_frameData.inFlightWorkerCommandBufferId];
				for (size_t i = 0; i < list.size(); i++) {
					const CommandBufferReleaseCall& call = list[i];
					call.second(call.first.second);
				}
				list.clear();
			}
		}

		// Run synchronisation jobs/events:
		{
			m_frameData.canGetWorkerCommandBuffer = true;
			WorkerCleanupList workerCleanupList(&data->workerCleanupLock, &data->workerCleanupJobs);
			{
				data->onPreSynch();
				workerCleanupList.Cleanup();
			}
			{
				data->synchJob.Execute(Device()->Log(), Callback<>(&WorkerCleanupList::Cleanup, &workerCleanupList));
			}
			{
				data->onSynch();
				workerCleanupList.Cleanup();
			}
			m_frameData.canGetWorkerCommandBuffer = false;
		}

		// Flush render jobs:
		{
			data->renderJob.jobSet.Flush(
				[&](const Reference<JobSystem::Job>* removed, size_t count) {
					for (size_t i = 0; i < count; i++)
						data->renderJob.jobSystem.Remove(removed[i]);
				}, [&](const Reference<JobSystem::Job>* added, size_t count) {
					for (size_t i = 0; i < count; i++)
						data->renderJob.jobSystem.Add(added[i]);
				});
		}

		// Flush renderer stack:
		{
			std::unique_lock<std::mutex> rendererLock(data->rendererLock);
			data->rendererSet.Flush([](const Reference<Renderer>*, size_t) {}, [](const Reference<Renderer>*, size_t) {});
			data->rendererStack.clear();
			for (size_t i = 0; i < data->rendererSet.Size(); i++)
				data->rendererStack.push_back(Data::RendererStackEntry(data->rendererSet[i]));
			std::sort(data->rendererStack.begin(), data->rendererStack.end());
			{
				std::unique_lock<SpinLock> lock(m_rendererStack.m_currentTargetTextureLock);
				data->rendererTargetTexture = m_rendererStack.TargetTexture();
			}
		}
	}
	void Scene::GraphicsContext::StartRender() {
		std::unique_lock<std::mutex> lock(m_renderThread.renderLock);
		Reference<Data> data = m_data;
		if (m_renderThread.rendering || (data == nullptr)) return;
		m_renderThread.startSemaphore.post();
		m_renderThread.rendering = true;
	}
	void Scene::GraphicsContext::SyncRender() {
		std::unique_lock<std::mutex> lock(m_renderThread.renderLock);
		Reference<Data> data = m_data;
		if ((!m_renderThread.rendering) || (data == nullptr)) return;
		m_renderThread.doneSemaphore.wait();
		m_renderThread.rendering = false;
	}


	Reference<Scene::GraphicsContext::Data> Scene::GraphicsContext::Data::Create(const Scene::GraphicsConstants* constants, OS::Logger* logger) {
		if (constants == nullptr) {
			logger->Error("Scene::GraphicsContext::Data::Create - null GraphicsConstants provided!");
			return nullptr;
		}

		Scene::GraphicsConstants graphicsConstants = *constants;

		if (graphicsConstants.shaderLoader == nullptr) {
			logger->Error("Scene::GraphicsContext::Data::Create - null ShaderLoader provided!");
			return nullptr;
		}

		if (graphicsConstants.graphicsDevice == nullptr) {
			logger->Warning("Scene::GraphicsContext::Data::Create - null graphics device provided! Creating one internally...");
			Reference<Application::AppInformation> appInfo = Object::Instantiate<Application::AppInformation>();
			Reference<Graphics::GraphicsInstance> graphicsInstance = Graphics::GraphicsInstance::Create(logger, appInfo);
			if (graphicsInstance == nullptr) {
				logger->Error("Scene::GraphicsContext::Data::Create - Failed to create graphics instance!");
				return nullptr;
			}
			else if (graphicsInstance->PhysicalDeviceCount() <= 0) {
				logger->Error("Scene::GraphicsContext::Data::Create - No physical devices detected!");
				return nullptr;
			}
			auto deviceViable = [&](Graphics::PhysicalDevice* device) {
				return
					device != nullptr &&
					device->HasFeature(Graphics::PhysicalDevice::DeviceFeature::GRAPHICS) &&
					device->HasFeature(Graphics::PhysicalDevice::DeviceFeature::COMPUTE) &&
					device->HasFeature(Graphics::PhysicalDevice::DeviceFeature::SYNCHRONOUS_COMPUTE) &&
					device->HasFeature(Graphics::PhysicalDevice::DeviceFeature::SAMPLER_ANISOTROPY); // We kinda need these features
			};
			{
				Graphics::PhysicalDevice* bestDevice = nullptr;
				for (size_t i = 0; i < graphicsInstance->PhysicalDeviceCount(); i++) {
					Graphics::PhysicalDevice* device = graphicsInstance->GetPhysicalDevice(i);
					if (!deviceViable(device)) continue;
					else if ((bestDevice != nullptr && bestDevice->Type() != Graphics::PhysicalDevice::DeviceType::VIRTUAL) &&
						(device->Type() == Graphics::PhysicalDevice::DeviceType::VIRTUAL)) continue; // We do not care about virtual devices
					else if (bestDevice == nullptr) bestDevice = device;
					else if (bestDevice->Type() < device->Type()) bestDevice = device;
					else if (bestDevice->Type() > device->Type()) continue;
					else if (
						(!bestDevice->HasFeature(Graphics::PhysicalDevice::DeviceFeature::ASYNCHRONOUS_COMPUTE)) &&
						device->HasFeature(Graphics::PhysicalDevice::DeviceFeature::ASYNCHRONOUS_COMPUTE)) bestDevice = device;
					else if (
						bestDevice->HasFeature(Graphics::PhysicalDevice::DeviceFeature::ASYNCHRONOUS_COMPUTE) &&
						(!device->HasFeature(Graphics::PhysicalDevice::DeviceFeature::ASYNCHRONOUS_COMPUTE))) continue;
					else if (bestDevice->VramCapacity() < device->VramCapacity()) bestDevice = device;
				}
				if (bestDevice == nullptr) {
					logger->Error("Scene::GraphicsContext::Data::Create - Failed to find a viable physical device!");
					return nullptr;
				}
				graphicsConstants.graphicsDevice = bestDevice->CreateLogicalDevice();
			}
			if (graphicsConstants.graphicsDevice == nullptr) {
				logger->Error("Scene::GraphicsContext::Data::Create - Failed to create the logical device!");
				for (size_t i = 0; i < graphicsInstance->PhysicalDeviceCount(); i++) {
					Graphics::PhysicalDevice* device = graphicsInstance->GetPhysicalDevice(i);
					if (!deviceViable(device)) continue;
					graphicsConstants.graphicsDevice = device->CreateLogicalDevice();
					if (graphicsConstants.graphicsDevice != nullptr) break;
				}
				if (graphicsConstants.graphicsDevice == nullptr) {
					logger->Error("Scene::GraphicsContext::Data::Create - Failed to create any logical device!");
					return nullptr;
				}
			}
		}

		if (graphicsConstants.lightSettings.lightTypeIds == nullptr) {
			logger->Warning("Scene::GraphicsContext::Data::Create - Light type identifiers not provided! Defaulting to built-in types.");
			graphicsConstants.lightSettings.lightTypeIds = &LightRegistry::JIMARA_BUILT_IN_LIGHT_IDENTIFIERS.typeIds;
			graphicsConstants.lightSettings.perLightDataSize = LightRegistry::JIMARA_BUILT_IN_LIGHT_IDENTIFIERS.perLightDataSize;
		}
		else if (graphicsConstants.lightSettings.perLightDataSize < LightRegistry::JIMARA_BUILT_IN_LIGHT_IDENTIFIERS.perLightDataSize)
			graphicsConstants.lightSettings.perLightDataSize = LightRegistry::JIMARA_BUILT_IN_LIGHT_IDENTIFIERS.perLightDataSize;

		return Object::Instantiate<Data>(graphicsConstants);
	}

	namespace {
		class RenderStackJob : public virtual JobSystem::Job {
		private:
			const Reference<Scene::GraphicsContext> m_context;
			Function<Graphics::TextureView*> m_getTargetTexture;
			Function<size_t> m_getStackSize;
			Function<Scene::GraphicsContext::Renderer*, size_t> m_getRenderer;

		public:
			inline RenderStackJob(
				Scene::GraphicsContext* context,
				const Function<Graphics::TextureView*>& getTargetTexture,
				const Function<size_t>& getStackSize,
				const Function<Scene::GraphicsContext::Renderer*, size_t>& getRenderer)
				: m_context(context)
				, m_getTargetTexture(getTargetTexture)
				, m_getStackSize(getStackSize)
				, m_getRenderer(getRenderer) {}

		protected:
			inline virtual void Execute() final override {
				Reference<Graphics::TextureView> view = m_getTargetTexture();
				if (view == nullptr) return;
				size_t count = m_getStackSize();
				const Graphics::Pipeline::CommandBufferInfo commandBufferInfo = m_context->GetWorkerThreadCommandBuffer();
				for (size_t i = 0; i < count; i++)
					m_getRenderer(i)->Render(commandBufferInfo, view);
			}

			inline virtual void CollectDependencies(Callback<Job*> addDependency) final override {
				size_t count = m_getStackSize();
				for (size_t i = 0; i < count; i++)
					m_getRenderer(i)->GetDependencies(addDependency);
			}
		};
	}

	Scene::GraphicsContext::Data::Data(const Scene::GraphicsConstants& constants)
		: context([&]() -> Reference<GraphicsContext> {
		Reference<GraphicsContext> ctx = new GraphicsContext(constants);
		ctx->ReleaseRef();
		return ctx;
			}()) {
		context->m_data.data = this;
		{
			Scene::GraphicsContext::Renderer* (*getRenderer)(const std::vector<RendererStackEntry>*, size_t) = 
				[](const std::vector<RendererStackEntry>* list, size_t index) -> Scene::GraphicsContext::Renderer* {
				return list->operator[](index).renderer;
			};
			Reference<RenderStackJob> job = Object::Instantiate<RenderStackJob>(
				context,
				Function<Graphics::TextureView*>(&Reference<Graphics::TextureView>::operator->, &rendererTargetTexture),
				Function<size_t>(&std::vector<RendererStackEntry>::size, &rendererStack),
				Function<Scene::GraphicsContext::Renderer*, size_t>(getRenderer, &rendererStack));
			context->RenderJobs().Add(job);
		}
		context->m_renderThread.renderThread = std::thread([](GraphicsContext* self) {
			while (true) {
				self->m_renderThread.startSemaphore.wait();
				Reference<Data> data = self->m_data;
				if (data == nullptr) break;
				{
					self->m_frameData.canGetWorkerCommandBuffer = true;
					WorkerCleanupList workerCleanupList(&data->workerCleanupLock, &data->workerCleanupJobs);
					{
						data->renderJob.jobSystem.Execute(
							self->Device()->Log(), Callback<>(&WorkerCleanupList::Cleanup, &workerCleanupList));
					}
					{
						data->onRenderFinished();
						workerCleanupList.Cleanup();
					}
					self->m_frameData.canGetWorkerCommandBuffer = false;
				}
				self->m_renderThread.doneSemaphore.post();
			}
			self->m_renderThread.doneSemaphore.post();
			}, context.operator->());
	}

	void Scene::GraphicsContext::Data::OnOutOfScope()const {
		std::unique_lock<std::mutex> renderLock(context->m_renderThread.renderLock);
		{
			std::unique_lock<SpinLock> dataLock(context->m_data.lock);
			if (RefCount() > 0) return;
			else context->m_data.data = nullptr;
		}
		if (context->m_renderThread.rendering)
			context->m_renderThread.doneSemaphore.wait();
		context->m_renderThread.startSemaphore.post();
		context->m_renderThread.renderThread.join();
		Object::OnOutOfScope();
	}
#ifndef USE_REFACTORED_SCENE
}
#endif
}

#include "GraphicsContext.h"


namespace Jimara {
	Scene::GraphicsContext::ConfigurationSettings::ConfigurationSettings(const CreateArgs& createArgs)
		: m_maxInFlightCommandBuffers(createArgs.graphics.maxInFlightCommandBuffers)
		, m_shaderLoader(createArgs.graphics.shaderLoader) {}

	Scene::GraphicsContext::BindlessSets::BindlessSets(
		const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>>& bindlessArrays,
		const Reference<Graphics::BindlessSet<Graphics::TextureSampler>>& bindlessSamplers,
		const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>& bindlessArrayInstance,
		const Reference<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>& bindlessSamplerInstance,
		size_t inFlightBufferCount)
		: m_bindlessArrays(bindlessArrays), m_bindlessSamplers(bindlessSamplers)
		, m_bindlessArrayInstance((bindlessArrayInstance != nullptr) ? bindlessArrayInstance : bindlessArrays->CreateInstance(inFlightBufferCount))
		, m_bindlessSamplerInstance((bindlessSamplerInstance != nullptr) ? bindlessSamplerInstance : bindlessSamplers->CreateInstance(inFlightBufferCount)) {}
	Scene::GraphicsContext::BindlessSets::BindlessSets(const CreateArgs& createArgs)
		: BindlessSets(
			(createArgs.graphics.bindlessResources.bindlessArrays != nullptr)
			? createArgs.graphics.bindlessResources.bindlessArrays : createArgs.graphics.graphicsDevice->CreateArrayBufferBindlessSet(),
			(createArgs.graphics.bindlessResources.bindlessSamplers != nullptr)
			? createArgs.graphics.bindlessResources.bindlessSamplers : createArgs.graphics.graphicsDevice->CreateTextureSamplerBindlessSet(),
			(createArgs.graphics.bindlessResources.bindlessArrays != nullptr) ? createArgs.graphics.bindlessResources.bindlessArrayBindings : nullptr,
			(createArgs.graphics.bindlessResources.bindlessSamplers != nullptr) ? createArgs.graphics.bindlessResources.bindlessSamplerBindings : nullptr,
			createArgs.graphics.maxInFlightCommandBuffers) {}

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
				m_lastQueryPool = GetCachedOrCreate(context, [&]() -> Reference<WorkerCommandPool> {
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

	Graphics::InFlightBufferInfo Scene::GraphicsContext::GetWorkerThreadCommandBuffer() {
		assert(this != nullptr);
		
		// Make sure we have a right to get the command buffer:
		Reference<Data> data = m_data;
		if (data == nullptr) {
			Device()->Log()->Error("Scene::GraphicsContext::GetWorkerThreadCommandBuffer - Scene out of scope!");
			return Graphics::InFlightBufferInfo(nullptr, 0);
		}
		else if (!m_frameData.canGetWorkerCommandBuffer) {
			Device()->Log()->Error("Scene::GraphicsContext::GetWorkerThreadCommandBuffer - Not a valid context to get a command buffer from!");
			return Graphics::InFlightBufferInfo(nullptr, 0);
		}

		// Get thread_local cache and WorkerCommandPoolCache:
		Reference<WorkerCommandPoolCache> cache = WorkerCommandPoolCache::ForThisThread();
		if (cache == nullptr) {
			Device()->Log()->Fatal(
				"Scene::GraphicsContext::GetWorkerThreadCommandBuffer - Failed to retrieve/create command pool cache! [File: '", __FILE__, "'; Line:", __LINE__, "]");
			return Graphics::InFlightBufferInfo(nullptr, 0);
		}

		// Get the worker command pool:
		WorkerCleanupList workerCleanupList(&data->workerCleanupLock, &data->workerCleanupJobs);
		Reference<WorkerCommandPool> commandPool = cache->GetFor(this, &workerCleanupList);
		if (commandPool == nullptr) {
			Device()->Log()->Fatal(
				"Scene::GraphicsContext::GetWorkerThreadCommandBuffer - Failed to retrieve/create command pool! [File: '", __FILE__, "'; Line:", __LINE__, "]");
			return Graphics::InFlightBufferInfo(nullptr, 0);
		}

		// Get command buffer:
		Reference<Graphics::CommandBuffer> commandBuffer = commandPool->GetCommandBuffer(workerCleanupList, [&](const CommandBufferReleaseCall& releaseCall) {
			std::unique_lock<SpinLock> lock(data->workerCleanupLock);
			data->inFlightBufferCleanupJobs[m_frameData.inFlightWorkerCommandBufferId].push_back(releaseCall);
			});
		return Graphics::InFlightBufferInfo(commandBuffer, m_frameData.inFlightWorkerCommandBufferId);
	}
	size_t Scene::GraphicsContext::InFlightCommandBufferIndex() { return m_frameData.inFlightWorkerCommandBufferId; }

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
	}

	Event<>& Scene::GraphicsContext::PreGraphicsSynch() {
		Reference<Data> data = m_data;
		if (data == nullptr) return EmptyEvent::Instance();
		else return data->onPreSynch;
	}

	JobSystem::JobSet& Scene::GraphicsContext::SynchPointJobs() {
		return m_synchPointJobs;
	}

	Event<>& Scene::GraphicsContext::OnGraphicsSynch() {
		Reference<Data> data = m_data;
		if (data == nullptr) return EmptyEvent::Instance();
		else return data->onSynch;
	}

	JobSystem::JobSet& Scene::GraphicsContext::RenderJobs() {
		return m_renderJobs;
	}

	Event<>& Scene::GraphicsContext::OnRenderFinished() {
		Reference<Data> data = m_data;
		if (data == nullptr) return EmptyEvent::Instance();
		else return data->onRenderFinished;
	}



	inline Scene::GraphicsContext::GraphicsContext(const CreateArgs& createArgs)
		: m_device(createArgs.graphics.graphicsDevice)
		, m_configuration(createArgs), m_bindlessSets(createArgs)
		, m_oneTimeCommandPool(Graphics::OneTimeCommandPool::GetFor(createArgs.graphics.graphicsDevice)) {
		m_synchPointJobs.context = this;
		m_renderJobs.context = this;
	}

	namespace {
		inline static void ReleaseCommandBuffers(CommandBufferReleaseList& list) {
			for (size_t i = 0; i < list.size(); i++) {
				const CommandBufferReleaseCall& call = list[i];
				call.second(call.first.second);
			}
			list.clear();
		}
	}

	void Scene::GraphicsContext::Sync(LogicContext* context) {
		Reference<Data> data = m_data;
		if (data == nullptr) return;
		
		// Increment in-flight buffer id:
		{
			std::unique_lock<SpinLock> lock(data->workerCleanupLock);
			m_frameData.inFlightWorkerCommandBufferId = (m_frameData.inFlightWorkerCommandBufferId + 1) % Configuration().MaxInFlightCommandBufferCount();
			if (data->inFlightBufferCleanupJobs.size() <= m_frameData.inFlightWorkerCommandBufferId)
				data->inFlightBufferCleanupJobs.resize(Configuration().MaxInFlightCommandBufferCount());
			else ReleaseCommandBuffers(data->inFlightBufferCleanupJobs[m_frameData.inFlightWorkerCommandBufferId]);
		}

		// Run synchronisation jobs/events:
		{
			m_frameData.canGetWorkerCommandBuffer = true;
			WorkerCleanupList workerCleanupList(&data->workerCleanupLock, &data->workerCleanupJobs);
			{
				data->onPreSynch();
				workerCleanupList.Cleanup();
				context->FlushComponentSets();
			}
			{
				data->synchJob.Execute(Device()->Log(), Callback<>(&WorkerCleanupList::Cleanup, &workerCleanupList));
				context->FlushComponentSets();
			}
			{
				data->onSynch();
				workerCleanupList.Cleanup();
				context->FlushComponentSets();
			}
			m_frameData.canGetWorkerCommandBuffer = false;
		}

		// Flush render jobs:
		{
			{
				std::unique_lock<std::mutex> lock(data->renderJob.setLock);
				data->renderJob.jobSet.Flush(
					[&](const Reference<JobSystem::Job>* removed, size_t count) {
						data->renderJob.removedJobBuffer.clear();
						for (size_t i = 0; i < count; i++)
							data->renderJob.removedJobBuffer.push_back(removed[i]);
					}, [&](const Reference<JobSystem::Job>* added, size_t count) {
						for (size_t i = 0; i < count; i++)
							data->renderJob.jobSystem.Add(added[i]);
					});
			}
			{
				const Reference<JobSystem::Job>* removed = data->renderJob.removedJobBuffer.data();
				const size_t count = data->renderJob.removedJobBuffer.size();
				for (size_t i = 0; i < count; i++)
					data->renderJob.jobSystem.Remove(removed[i]);
				data->renderJob.removedJobBuffer.clear();
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


	Reference<Scene::GraphicsContext::Data> Scene::GraphicsContext::Data::Create(CreateArgs& createArgs) {
		if (createArgs.graphics.shaderLoader == nullptr) {
			if (createArgs.createMode == CreateArgs::CreateMode::CREATE_DEFAULT_FIELDS_AND_WARN)
				createArgs.logic.logger->Warning("Scene::GraphicsContext::Data::Create - null ShaderLoader provided! Defaulting to ShaderDirectoryLoader('Shaders')");
			else if (createArgs.createMode == CreateArgs::CreateMode::ERROR_ON_MISSING_FIELDS) {
				createArgs.logic.logger->Error("Scene::GraphicsContext::Data::Create - null ShaderLoader provided!");
				return nullptr;
			}
			createArgs.graphics.shaderLoader = Graphics::ShaderDirectoryLoader::Create("Shaders", createArgs.logic.logger);
		}

		if (createArgs.graphics.graphicsDevice == nullptr) {
			if (createArgs.createMode == CreateArgs::CreateMode::CREATE_DEFAULT_FIELDS_AND_WARN)
				createArgs.logic.logger->Warning("Scene::GraphicsContext::Data::Create - null graphics device provided! Creating one internally...");
			else if (createArgs.createMode == CreateArgs::CreateMode::ERROR_ON_MISSING_FIELDS) {
				createArgs.logic.logger->Error("Scene::GraphicsContext::Data::Create - null graphics device provided!");
				return nullptr;
			}
			Reference<Application::AppInformation> appInfo = Object::Instantiate<Application::AppInformation>();
			Reference<Graphics::GraphicsInstance> graphicsInstance = Graphics::GraphicsInstance::Create(createArgs.logic.logger, appInfo);
			if (graphicsInstance == nullptr) {
				createArgs.logic.logger->Error("Scene::GraphicsContext::Data::Create - Failed to create graphics instance!");
				return nullptr;
			}
			else if (graphicsInstance->PhysicalDeviceCount() <= 0) {
				createArgs.logic.logger->Error("Scene::GraphicsContext::Data::Create - No physical devices detected!");
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
					createArgs.logic.logger->Error("Scene::GraphicsContext::Data::Create - Failed to find a viable physical device!");
					return nullptr;
				}
				createArgs.graphics.graphicsDevice = bestDevice->CreateLogicalDevice();
			}
			if (createArgs.graphics.graphicsDevice == nullptr) {
				createArgs.logic.logger->Error("Scene::GraphicsContext::Data::Create - Failed to create the logical device!");
				for (size_t i = 0; i < graphicsInstance->PhysicalDeviceCount(); i++) {
					Graphics::PhysicalDevice* device = graphicsInstance->GetPhysicalDevice(i);
					if (!deviceViable(device)) continue;
					createArgs.graphics.graphicsDevice = device->CreateLogicalDevice();
					if (createArgs.graphics.graphicsDevice != nullptr) break;
				}
				if (createArgs.graphics.graphicsDevice == nullptr) {
					createArgs.logic.logger->Error("Scene::GraphicsContext::Data::Create - Failed to create any logical device!");
					return nullptr;
				}
			}
		}

		return Object::Instantiate<Data>(createArgs);
	}



	Scene::GraphicsContext::Data::Data(const CreateArgs& createArgs)
		: context([&]() -> Reference<GraphicsContext> {
		Reference<GraphicsContext> ctx = new GraphicsContext(createArgs);
		ctx->ReleaseRef();
		return ctx;
			}())
		, synchJob(createArgs.graphics.synchPointThreadCount <= 0 
			? max((size_t)1, (size_t)std::thread::hardware_concurrency()) : createArgs.graphics.synchPointThreadCount)
		, renderJob(createArgs.graphics.renderThreadCount <= 0 
			? max((size_t)1, (size_t)std::thread::hardware_concurrency() / (size_t)2) : createArgs.graphics.renderThreadCount) {
		context->m_data.data = this;
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
		AddRef();
		Scene::GraphicsContext::Data* self;
		{
			std::unique_lock<std::mutex> renderLock(context->m_renderThread.renderLock);
			{
				std::unique_lock<SpinLock> dataLock(context->m_data.lock);
				if (RefCount() > 1) {
					ReleaseRef();
					return;
				}
				else {
					self = context->m_data.data;
					context->m_data.data = nullptr;
				}
			}
			if (context->m_renderThread.rendering)
				context->m_renderThread.doneSemaphore.wait();
			context->m_renderThread.startSemaphore.post();
			context->m_renderThread.renderThread.join();
		}
		if (self != nullptr) {
			std::unique_lock<SpinLock> lock(self->workerCleanupLock);
			for (size_t i = 0; i < inFlightBufferCleanupJobs.size(); i++)
				ReleaseCommandBuffers(self->inFlightBufferCleanupJobs[i]);
		}
		Object::OnOutOfScope();
	}
}

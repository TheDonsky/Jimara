#include "GraphicsContext.h"


namespace Jimara {
namespace Refactor_TMP_Namespace {
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



	inline Scene::GraphicsContext::GraphicsContext(Graphics::GraphicsDevice* device) 
		: m_device(device), m_rendererStack(this) {}

	void Scene::GraphicsContext::Sync() {
		Reference<Data> data = m_data;
		if (data == nullptr) return;
		data->onPreSynch();
		data->synchJob.Execute();
		data->onSynch();
		data->renderJob.jobSet.Flush(
			[&](const Reference<JobSystem::Job>* removed, size_t count) {
				for (size_t i = 0; i < count; i++)
					data->renderJob.jobSystem.Remove(removed[i]);
			}, [&](const Reference<JobSystem::Job>* added, size_t count) {
				for (size_t i = 0; i < count; i++)
					data->renderJob.jobSystem.Add(added[i]);
			});
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


	Reference<Scene::GraphicsContext::Data> Scene::GraphicsContext::Data::Create(Graphics::GraphicsDevice* device, OS::Logger* logger) {
		Reference<Graphics::GraphicsDevice> graphicsDevice;
		if (graphicsDevice == nullptr) {
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
				graphicsDevice = bestDevice->CreateLogicalDevice();
			}
			if (graphicsDevice == nullptr) {
				logger->Error("Scene::GraphicsContext::Data::Create - Failed to create the logical device!");
				for (size_t i = 0; i < graphicsInstance->PhysicalDeviceCount(); i++) {
					Graphics::PhysicalDevice* device = graphicsInstance->GetPhysicalDevice(i);
					if (!deviceViable(device)) continue;
					graphicsDevice = device->CreateLogicalDevice();
					if (graphicsDevice != nullptr) break;
				}
				if (graphicsDevice == nullptr) {
					logger->Error("Scene::GraphicsContext::Data::Create - Failed to create any logical device!");
					return nullptr;
				}
			}
		}
		return Object::Instantiate<Data>(graphicsDevice);
	}

	namespace {
		class RenderStackJob : public virtual JobSystem::Job {
		private:
			Function<Graphics::TextureView*> m_getTargetTexture;
			Function<size_t> m_getStackSize;
			Function<Scene::GraphicsContext::Renderer*, size_t> m_getRenderer;

		public:
			inline RenderStackJob(
				const Function<Graphics::TextureView*>& getTargetTexture,
				const Function<size_t>& getStackSize,
				const Function<Scene::GraphicsContext::Renderer*, size_t>& getRenderer)
				: m_getTargetTexture(getTargetTexture)
				, m_getStackSize(getStackSize)
				, m_getRenderer(getRenderer) {}

		protected:
			inline virtual void Execute() final override {
				Reference<Graphics::TextureView> view = m_getTargetTexture();
				if (view == nullptr) return;
				size_t count = m_getStackSize();
				for (size_t i = 0; i < count; i++)
					m_getRenderer(i)->Render({ /* __TODO__: Actually assign the command buffer and id */ }, view);
			}

			inline virtual void CollectDependencies(Callback<Job*> addDependency) final override {
				size_t count = m_getStackSize();
				for (size_t i = 0; i < count; i++)
					m_getRenderer(i)->GetDependencies(addDependency);
			}
		};
	}

	Scene::GraphicsContext::Data::Data(Graphics::GraphicsDevice* device)
		: context([&]() -> Reference<GraphicsContext> {
		Reference<GraphicsContext> ctx = new GraphicsContext(device);
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
				data->renderJob.jobSystem.Execute();
				data->onRenderFinished();
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
}
}

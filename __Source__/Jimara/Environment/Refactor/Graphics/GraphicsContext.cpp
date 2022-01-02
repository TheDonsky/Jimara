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

	JobSystem::JobSet& Scene::GraphicsContext::SyncPointJobs() {
		Reference<Data> data = m_data;
		if (data != nullptr) return data->syncJob.Jobs();
		else return EmptyJobSet::Instance();
	}

	JobSystem::JobSet& Scene::GraphicsContext::RenderJobs() {
		Reference<Data> data = m_data;
		if (data != nullptr) return data->renderJob;
		else return EmptyJobSet::Instance();
	}


	void Scene::GraphicsContext::Sync() {
		Reference<Data> data = m_data;
		if (data == nullptr) return;
		// __TODO__: Maybe we nned an event or two here?
		data->syncJob.Execute();
		// __TODO__: Flush scene object addition/removal
		data->renderJob.jobSet.Flush(
			[&](const Reference<JobSystem::Job>* removed, size_t count) {
				for (size_t i = 0; i < count; i++)
					data->renderJob.jobSystem.Remove(removed[i]);
			}, [&](const Reference<JobSystem::Job>* added, size_t count) {
				for (size_t i = 0; i < count; i++)
					data->renderJob.jobSystem.Add(added[i]);
			});
		
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

	Scene::GraphicsContext::Data::Data(Graphics::GraphicsDevice* device)
		: context([&]() -> Reference<GraphicsContext> {
		Reference<GraphicsContext> ctx = new GraphicsContext(device);
		ctx->ReleaseRef();
		return ctx;
			}()) {
		context->m_data = this;
		context->m_renderThread.renderThread = std::thread([](GraphicsContext* self) {
			while (true) {
				self->m_renderThread.startSemaphore.wait();
				Reference<Data> data = self->m_data;
				if (data == nullptr) break;
				data->renderJob.jobSystem.Execute();
				self->m_renderThread.doneSemaphore.post();
			}
			self->m_renderThread.doneSemaphore.post();
			}, context.operator->());
	}

	Scene::GraphicsContext::Data::~Data() {
		std::unique_lock<std::mutex> lock(context->m_renderThread.renderLock);
		if (context->m_renderThread.rendering)
			context->m_renderThread.doneSemaphore.wait();
		context->m_data = nullptr;
		context->m_renderThread.startSemaphore.post();
		context->m_renderThread.renderThread.join();
	}
}
}

#include "RayTracedRenderer_Tools.h"
#include "../ForwardRendering/ForwardPlusLightingModel.h"
#include "../../SceneObjects/Lights/LightmapperJobs.h"
#include "../../../GraphicsSimulation/GraphicsSimulation.h"


namespace Jimara {
	class RayTracedRenderer::Tools::Renderer : public virtual RenderStack::Renderer {
	public:
		inline Renderer(
			LightmapperJobs* lightmapperJobs,
			GraphicsSimulation::JobDependencies* simulationJobs,
			FrameBufferManager* frameBuffers,
			Tools::SharedBindings* sharedBindings,
			Tools::SceneObjectData* sceneObjectData,
			RasterPass* rasterPass, 
			RayTracedPass* rtPass) 
			: m_lightmapperJobs(lightmapperJobs)
			, m_graphicsSimulation(simulationJobs)
			, m_frameBuffers(frameBuffers)
			, m_sharedBindings(sharedBindings)
			, m_sceneObjectData(sceneObjectData)
			, m_rasterPass(rasterPass)
			, m_rtPass(rtPass)
			, m_inFlightResources(Object::Instantiate<InFlightResources>(sharedBindings->viewport->Context())) {
			assert(m_frameBuffers != nullptr);
			assert(m_sharedBindings != nullptr);
			assert(m_rasterPass != nullptr);
			assert(m_rtPass != nullptr);
			assert(m_inFlightResources != nullptr);
			m_inFlightResources->context->Graphics()->RenderJobs().Add(m_inFlightResources);
		}

		inline virtual ~Renderer() {
			m_inFlightResources->context->Graphics()->RenderJobs().Remove(m_inFlightResources);
		}

		inline virtual void Render(Graphics::InFlightBufferInfo commandBufferInfo, RenderImages* images) override {
			// Resource-Lists:
			std::unique_lock<std::mutex> resourceListLock(m_inFlightResources->lock);
			while (m_inFlightResources->resources.size() <= commandBufferInfo.inFlightBufferId)
				m_inFlightResources->resources.push_back({});
			std::vector<Reference<const Object>>& inFlightResourceList = m_inFlightResources->resources[commandBufferInfo.inFlightBufferId];
			inFlightResourceList.clear();

			FrameBufferManager::Lock frameBuffers(m_frameBuffers, images);
			if (!frameBuffers.Good())
				return;

			m_sharedBindings->Update();

			// Set frame buffers:
			if (!m_rasterPass->SetFrameBuffers(frameBuffers.Buffers()))
				return;
			if (!m_rtPass->SetFrameBuffers(frameBuffers.Buffers()))
				return;

			Tools::RasterPass::State rasterState(m_rasterPass);

			// Update scene object data:
			if (!m_sceneObjectData->Update(rasterState.Pipelines(), m_rtPass, inFlightResourceList))
				return;

			// TODO: 
			// . Once frame buffers are set, obtain graphics object descriptor list; 
			// . If there's any BLAS that needs to be built, we should aknowelege and schedule the build/rebuild process 
			// either immediately or in a separate task-thread, depending on our requirenments and per-object flags.
			// . Generate per-instance info buffer for each index; Give that buffer to both the raster pass and the RT pass;
			// Instance info will contain lit-shader and material indices alongside some flags and vertex input layout data.

			if (!rasterState.Render(commandBufferInfo))
				return;

			
			if (!m_rtPass->Render(commandBufferInfo))
				return;
		}

		inline virtual void GetDependencies(Callback<JobSystem::Job*> report) override {
			report(m_sharedBindings->lightDataBuffer);
			report(m_sharedBindings->lightTypeIdBuffer);
			report(m_sharedBindings->lightGrid->UpdateJob());
			m_lightmapperJobs->GetAll(report);
			m_graphicsSimulation->CollectDependencies(report);
			m_rasterPass->GetDependencies(report);
			m_rtPass->GetDependencies(report);
			report(m_inFlightResources);
		}

	private:
		// Dependencies:
		const Reference<LightmapperJobs> m_lightmapperJobs;
		const Reference<GraphicsSimulation::JobDependencies> m_graphicsSimulation;

		// Shared buffers:
		const Reference<FrameBufferManager> m_frameBuffers;
		const Reference<Tools::SharedBindings> m_sharedBindings;
		const Reference<Tools::SceneObjectData> m_sceneObjectData;

		// Underlying passes:
		const Reference<RasterPass> m_rasterPass;
		const Reference<RayTracedPass> m_rtPass;

		// Resources for each in-flight frame:
		struct InFlightResources : public virtual JobSystem::Job {
			const Reference<SceneContext> context;
			std::mutex lock;
			std::vector<std::vector<Reference<const Object>>> resources;

			inline InFlightResources(SceneContext* ctx) : context(ctx) {
				std::unique_lock<std::mutex> listLock(lock);
				resources.resize(context->Graphics()->Configuration().MaxInFlightCommandBufferCount());
			}

			virtual ~InFlightResources() {}

			virtual void Execute() final override {
				resources[context->Graphics()->InFlightCommandBufferIndex()].clear();
			}

			virtual void CollectDependencies(Callback<Job*> addDependency) final override {}
		};
		const Reference<InFlightResources> m_inFlightResources;

		// Private stuff resides in-here
		struct Helpers;
	};

	Reference<RenderStack::Renderer> RayTracedRenderer::CreateRenderer(
		const ViewportDescriptor* viewport, LayerMask layers, Graphics::RenderPass::Flags flags)const {
		Unused(flags); // __TODO__: Actually incorporate this!

		if (viewport == nullptr || viewport->Context() == nullptr)
			return nullptr;

		if (!viewport->Context()->Graphics()->Device()->PhysicalDevice()->HasFeatures(Graphics::PhysicalDevice::DeviceFeatures::RAY_TRACING)) {
			viewport->Context()->Log()->Warning("RayTracedRenderer::CreateRenderer - "
				"Device does not support Hardware-accelerated Ray-Tracing! Falling back to a Forward-Plus renderer!");
			return ForwardPlusLightingModel::Instance()->CreateRenderer(viewport, layers, flags);
		}

		auto fail = [&](const auto... message) {
			viewport->Context()->Log()->Error("RayTracedRenderer::CreateRenderer - ", message...);
			return nullptr;
		};

		const Reference<LightmapperJobs> lightmapperJobs = LightmapperJobs::GetInstance(viewport->Context());
		if (lightmapperJobs == nullptr)
			return fail("Failed to get lightmapper jobs! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<GraphicsSimulation::JobDependencies> simulationJobs = GraphicsSimulation::JobDependencies::For(viewport->Context());
		if (simulationJobs == nullptr)
			return fail("Failed to get simulation job dependencies! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Tools::FrameBufferManager> frameBufferManager = Object::Instantiate<Tools::FrameBufferManager>(viewport->Context());
		if (frameBufferManager == nullptr)
			return nullptr;

		const Reference<Tools::SharedBindings> sharedBindings = Tools::SharedBindings::Create(viewport);
		if (sharedBindings == nullptr)
			return nullptr;

		const Reference<Tools::SceneObjectData> sceneObjectData = Object::Instantiate<Tools::SceneObjectData>(sharedBindings);
		if (sceneObjectData == nullptr)
			return nullptr;

		const Reference<Tools::RasterPass> rasterPass = Tools::RasterPass::Create(this, sharedBindings, layers, flags);
		if (rasterPass == nullptr)
			return nullptr;

		const Reference<Tools::RayTracedPass> rtPasss = Tools::RayTracedPass::Create(this, sharedBindings, layers);
		if (rtPasss == nullptr)
			return nullptr;

		return Object::Instantiate<Tools::Renderer>(lightmapperJobs, simulationJobs, frameBufferManager, sharedBindings, sceneObjectData, rasterPass, rtPasss);
	}

	void RayTracedRenderer::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		// __TODO__: Implement this crap!
		Unused(recordElement);
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<RayTracedRenderer>(const Callback<const Object*>& report) {
		static const Reference<ConfigurableResource::ResourceFactory> factory = ConfigurableResource::ResourceFactory::Create<RayTracedRenderer>(
			"Ray-traced Lighting Model", "Jimara/Rendering/Lighting Models/Ray-Traced", "Ray-traced lighting model");
		report(factory);
	}
}

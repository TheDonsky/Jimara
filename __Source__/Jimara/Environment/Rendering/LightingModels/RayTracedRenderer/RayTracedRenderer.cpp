#include "RayTracedRenderer_Tools.h"
#include "../ForwardRendering/ForwardPlusLightingModel.h"
#include "../../SceneObjects/Lights/LightmapperJobs.h"
#include "../../../GraphicsSimulation/GraphicsSimulation.h"
#include "../../../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../../../Data/Serialization/Attributes/EnumAttribute.h"
#include "../../../../Data/Serialization/Attributes/SliderAttribute.h"


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
			, m_rtPass(rtPass) {
			assert(m_frameBuffers != nullptr);
			assert(m_sharedBindings != nullptr);
			assert(m_rasterPass != nullptr);
			assert(m_rtPass != nullptr);
		}

		inline virtual ~Renderer() {}

		inline virtual void Render(Graphics::InFlightBufferInfo commandBufferInfo, RenderImages* images) override {
			// Resource-Lists:
			std::unique_lock<std::mutex> resourceListLock(m_renderLock);
			
			FrameBufferManager::Lock frameBuffers(m_frameBuffers, images);
			if (!frameBuffers.Good())
				return;

			// Set frame buffers:
			if (!m_rasterPass->SetFrameBuffers(frameBuffers.Buffers()))
				return;
			if (!m_rtPass->SetFrameBuffers(frameBuffers.Buffers()))
				return;

			Tools::RasterPass::State rasterState(m_rasterPass);
			Tools::RayTracedPass::State rtState(m_rtPass);

			// Update scene object data:
			{
				m_inFlightResourceList.clear();
				const bool updateSuccess = m_sceneObjectData->Update(rasterState.Pipelines(), rtState, m_inFlightResourceList);
				commandBufferInfo.commandBuffer->AddDependencies(m_inFlightResourceList);
				m_inFlightResourceList.clear();
				if (!updateSuccess)
					return;
			}

			// Update shared bindings:
			m_sharedBindings->Update(m_sceneObjectData->RasterizedGeometrySize());

			// Render v-buffer:
			if ((m_sharedBindings->viewportBufferData.renderFlags & RendererFlags::USE_RASTER_VBUFFER) != RendererFlags::NONE)
				if (!rasterState.Render(commandBufferInfo))
					return;

			// Run RT pipeline:
			if (!rtState.Render(commandBufferInfo))
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
		std::mutex m_renderLock;
		std::vector<Reference<const Object>> m_inFlightResourceList;
		
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

		const Reference<Tools::AccelerationStructureViewportDesc> tlasViewport =
			Object::Instantiate<Tools::AccelerationStructureViewportDesc>(viewport);

		const Reference<LightmapperJobs> lightmapperJobs = LightmapperJobs::GetInstance(viewport->Context());
		if (lightmapperJobs == nullptr)
			return fail("Failed to get lightmapper jobs! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<GraphicsSimulation::JobDependencies> simulationJobs = GraphicsSimulation::JobDependencies::For(viewport->Context());
		if (simulationJobs == nullptr)
			return fail("Failed to get simulation job dependencies! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Tools::FrameBufferManager> frameBufferManager = Object::Instantiate<Tools::FrameBufferManager>(viewport->Context());
		if (frameBufferManager == nullptr)
			return nullptr;

		const Reference<Tools::SharedBindings> sharedBindings = Tools::SharedBindings::Create(this, tlasViewport);
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


	const Object* RayTracedRenderer::RendererFlagsEnumAttribute() {
		using AttributeT = Serialization::EnumAttribute<std::underlying_type_t<RendererFlags>>;
		static const Reference<const AttributeT> attribute = Object::Instantiate<AttributeT>(true,
			"USE_RASTER_VBUFFER", RendererFlags::USE_RASTER_VBUFFER,
			"FALLBACK_ON_FIRST_RAY_IF_VBUFFER_EVAL_FAILS", RendererFlags::FALLBACK_ON_FIRST_RAY_IF_VBUFFER_EVAL_FAILS,
			"DISCARD_IRRADIANCE_PHOTONS_IF_RAY_DEPTH_THRESHOLD_REACHED", RendererFlags::DISCARD_IRRADIANCE_PHOTONS_IF_RAY_DEPTH_THRESHOLD_REACHED,
			"SCALE_ACCELERATION_STRUCTURE_RANGE_BY_FAR_PLANE", RendererFlags::SCALE_ACCELERATION_STRUCTURE_RANGE_BY_FAR_PLANE);
		return attribute;
	}

	void RayTracedRenderer::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Flags, SetFlags, "Flags", "Flags, controlling various parts of the render process.", RendererFlagsEnumAttribute());
			JIMARA_SERIALIZE_FIELD_GET_SET(AccelerationStructureRange, SetAccelerationStructureRange, "Acceleration Structure Range",
				"Range, for how far the acceleration structure 'sees'; "
				"If SCALE_ACCELERATION_STRUCTURE_RANGE_BY_FAR_PLANE is used, this value will be understood as a fraction of the rendering viewport's far plane.");
			JIMARA_SERIALIZE_FIELD_GET_SET(MaxTraceDepth, SetMaxTraceDepth, "Max Trace Depth", "Maximal number of indirect bounces, a ray can take per pixel.");
			JIMARA_SERIALIZE_FIELD_GET_SET(IndirectRoughnessThreshold, SetIndirectRoughnessThreshold, "Roughness Threshold",
				"Maximal roughness, beyond which indirect samples will not be requested.",
				Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 1.0f));
			JIMARA_SERIALIZE_FIELD_GET_SET(BounceTransmittanceThreshold, SetBounceTransmittanceThreshold, "Bounce Transmittance Threshold",
				"If bounce sample request throughput (in terms of total per-pixel contribution) is less than this threshold, the requested sample will be ignored.",
				Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 1.0f));
			JIMARA_SERIALIZE_FIELD_GET_SET(MaxSamplesPerPixel, SetMaxSamplesPerPixel, "Samples Per Pixel", 
				"Traced samples per-pixel-per-frame rendered (debug purposes only; averages-out frame buffer if it is not cleared and viewport stays static).");
		};
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<RayTracedRenderer>(const Callback<const Object*>& report) {
		static const Reference<ConfigurableResource::ResourceFactory> factory = ConfigurableResource::ResourceFactory::Create<RayTracedRenderer>(
			"Ray-traced Lighting Model", "Jimara/Rendering/Lighting Models/Ray-Traced", "Ray-traced lighting model");
		report(factory);
	}
}

#include "RayTracedRenderer_Tools.h"
#include "../ForwardRendering/ForwardPlusLightingModel.h"


namespace Jimara {
	class RayTracedRenderer::Tools::Renderer : public virtual RenderStack::Renderer {
	public:
		inline Renderer(
			FrameBufferManager* frameBuffers,
			RasterPass* rasterPass, 
			RayTracedPass* rtPass) 
			: m_frameBuffers(frameBuffers)
			, m_rasterPass(rasterPass)
			, m_rtPass(rtPass) {
			assert(m_frameBuffers != nullptr);
			assert(m_rasterPass != nullptr);
			assert(m_rtPass != nullptr);
		}

		inline virtual ~Renderer() {

		}

		inline virtual void Render(Graphics::InFlightBufferInfo commandBufferInfo, RenderImages* images) override {
			FrameBufferManager::Lock frameBuffers(m_frameBuffers, images);
			if (!frameBuffers.Good())
				return;
			
			if (!m_rasterPass->SetFrameBuffers(frameBuffers.Buffers()))
				return;

			// TODO: 
			// . Once frame buffers are set, obtain graphics object descriptor list; 
			// . If there's any BLAS that needs to be built, we should aknowelege and schedule the build/rebuild process 
			// either immediately or in a separate task-thread, depending on our requirenments and per-object flags.
			// . Generate per-instance info buffer for each index; Give that buffer to both the raster pass and the RT pass;
			// Instance info will contain lit-shader and material indices alongside some flags and vertex input layout data.

			if (!m_rasterPass->Render(commandBufferInfo))
				return;

			if (!m_rtPass->SetFrameBuffers(frameBuffers.Buffers()))
				return;
			if (!m_rtPass->Render(commandBufferInfo))
				return;
		}

		inline virtual void GetDependencies(Callback<JobSystem::Job*> report) override {
			m_rasterPass->GetDependencies(report);
			m_rtPass->GetDependencies(report);
		}

	private:
		// Shared buffers:
		const Reference<FrameBufferManager> m_frameBuffers;

		// Underlying passes:
		const Reference<RasterPass> m_rasterPass;
		const Reference<RayTracedPass> m_rtPass;

		// Private stuff resides in-here
		struct Helpers;
	};

	Reference<RenderStack::Renderer> RayTracedRenderer::CreateRenderer(
		const ViewportDescriptor* viewport, LayerMask layers, Graphics::RenderPass::Flags flags)const {
		Unused(flags); // __TODO__: Actually incorporate this!

		if (viewport == nullptr || viewport->Context() == nullptr)
			return nullptr;

		if (!viewport->Context()->Graphics()->Device()->PhysicalDevice()->HasFeatures(Graphics::PhysicalDevice::DeviceFeatures::RAY_TRACING)) {
			viewport->Context()->Log()->Warning("RayTracedRenderer::CreateRenderer - Device does not support Hardware-accelerated Ray-Tracing! Falling back to a Forward-Plus renderer!");
			return ForwardPlusLightingModel::Instance()->CreateRenderer(viewport, layers, flags);
		}

		const Reference<Tools::FrameBufferManager> frameBufferManager = Object::Instantiate<Tools::FrameBufferManager>(viewport->Context());
		if (frameBufferManager == nullptr)
			return nullptr;

		const Reference<Tools::RasterPass> rasterPass = Tools::RasterPass::Create(this, viewport, layers, flags);
		if (rasterPass == nullptr)
			return nullptr;

		const Reference<Tools::RayTracedPass> rtPasss = Tools::RayTracedPass::Create(this, viewport, layers);
		if (rtPasss == nullptr)
			return nullptr;

		return Object::Instantiate<Tools::Renderer>(frameBufferManager, rasterPass, rtPasss);
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

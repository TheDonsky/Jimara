#include "RayTracedRenderer_Tools.h"
#include "../ForwardRendering/ForwardPlusLightingModel.h"


namespace Jimara {
	class RayTracedRenderer::Tools::Renderer : public virtual RenderStack::Renderer {
	public:
		inline Renderer(
			SharedDataManager* sharedData, 
			RasterPass* rasterPass, 
			RayTracedPass* rtPass) 
			: m_sharedData(sharedData)
			, m_rasterPass(rasterPass)
			, m_rtPass(rtPass) {
			assert(m_sharedData != nullptr);
			assert(m_rasterPass != nullptr);
			assert(m_rtPass != nullptr);
		}

		inline virtual ~Renderer() {

		}

		inline virtual void Render(Graphics::InFlightBufferInfo commandBufferInfo, RenderImages* images) override {
			SharedData data = m_sharedData->StartPass(images);
			if (!data)
				return;
			m_rasterPass->Render(commandBufferInfo, data);
			m_rtPass->Render(commandBufferInfo, data);
		}

		inline virtual void GetDependencies(Callback<JobSystem::Job*> report) override {
			m_rasterPass->GetDependencies(report);
			m_rtPass->GetDependencies(report);
		}

	private:
		// Shared buffers:
		const Reference<SharedDataManager> m_sharedData;

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

		const Reference<Tools::SharedDataManager> sharedData = Tools::SharedDataManager::Create(this, viewport, layers);
		if (sharedData == nullptr)
			return nullptr;

		const Reference<Tools::RasterPass> rasterPass = Tools::RasterPass::Create(this, viewport, layers);
		if (rasterPass == nullptr)
			return nullptr;

		const Reference<Tools::RayTracedPass> rtPasss = Tools::RayTracedPass::Create(this, viewport, layers);
		if (rtPasss == nullptr)
			return nullptr;

		return Object::Instantiate<Tools::Renderer>(sharedData, rasterPass, rtPasss);
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

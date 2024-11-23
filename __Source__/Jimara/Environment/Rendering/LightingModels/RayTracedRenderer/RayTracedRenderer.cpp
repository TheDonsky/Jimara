#include "RayTracedRenderer.h"


namespace Jimara {
	Reference<RenderStack::Renderer> RayTracedRenderer::CreateRenderer(const ViewportDescriptor* viewport, LayerMask layers, Graphics::RenderPass::Flags flags)const {
		// __TODO__: Implement this crap!
		if (viewport == nullptr)
			return nullptr;
		viewport->Context()->Log()->Error("RayTracedRenderer::CreateRenderer - Not yet implemented!");
		return nullptr;
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

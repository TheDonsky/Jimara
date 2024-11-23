#include "RayTracedRenderer_Tools.h"


namespace Jimara {
	struct RayTracedRenderer::Tools::RayTracedPass::Helpers {
		// __TODO__: Implement this crap!
	};

	RayTracedRenderer::Tools::RayTracedPass::RayTracedPass() {
		// __TODO__: Implement this crap!
	}

	RayTracedRenderer::Tools::RayTracedPass::~RayTracedPass() {
		// __TODO__: Implement this crap!
	}

	Reference<RayTracedRenderer::Tools::RayTracedPass> RayTracedRenderer::Tools::RayTracedPass::Create(
		const RayTracedRenderer* renderer, const ViewportDescriptor* viewport, LayerMask layers) {
		viewport->Context()->Log()->Error(__FILE__, ": ", __LINE__, " Not yet implemented!");
		return nullptr;
	}

	void RayTracedRenderer::Tools::RayTracedPass::Render(Graphics::InFlightBufferInfo commandBufferInfo, const SharedData& data) {
		// __TODO__: Implement this crap!
	}

	void RayTracedRenderer::Tools::RayTracedPass::GetDependencies(Callback<JobSystem::Job*> report) {
		// __TODO__: Implement this crap!
	}
}

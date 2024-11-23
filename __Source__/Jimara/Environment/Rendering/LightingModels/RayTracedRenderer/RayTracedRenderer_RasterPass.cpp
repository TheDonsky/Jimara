#include "RayTracedRenderer_Tools.h"


namespace Jimara {
	struct RayTracedRenderer::Tools::RasterPass::Helpers {
		// __TODO__: Implement this crap!
	};

	RayTracedRenderer::Tools::RasterPass::RasterPass() {
		// __TODO__: Implement this crap!
	}

	RayTracedRenderer::Tools::RasterPass::~RasterPass() {
		// __TODO__: Implement this crap!
	}

	Reference<RayTracedRenderer::Tools::RasterPass> RayTracedRenderer::Tools::RasterPass::Create(
		const RayTracedRenderer* renderer, const ViewportDescriptor* viewport, LayerMask layers) {
		viewport->Context()->Log()->Error(__FILE__, ": ", __LINE__, " Not yet implemented!");
		return nullptr;
	}

	void RayTracedRenderer::Tools::RasterPass::Render(Graphics::InFlightBufferInfo commandBufferInfo, const SharedData& data) {
		// __TODO__: Implement this crap!
	}

	void RayTracedRenderer::Tools::RasterPass::GetDependencies(Callback<JobSystem::Job*> report) {
		// __TODO__: Implement this crap!
	}
}

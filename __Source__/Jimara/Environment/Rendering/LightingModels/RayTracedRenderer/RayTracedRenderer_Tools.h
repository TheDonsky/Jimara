#pragma once
#include "RayTracedRenderer.h"


namespace Jimara {
	struct RayTracedRenderer::Tools {
		class RasterPass;
	};

	class RayTracedRenderer::Tools::RasterPass : public virtual Object {
	public:
		virtual ~RasterPass();

		static Reference<RasterPass> Create(const RayTracedRenderer* renderer, const ViewportDescriptor* viewport, LayerMask layers);

		void Render(Graphics::InFlightBufferInfo commandBufferInfo, RenderImages* images);

		void GetDependencies(Callback<JobSystem::Job*> report);

	private:
		// Constructor can only be invoked internally..
		RasterPass();

		// Private stuff resides in-here
		struct Helpers;
	};
}

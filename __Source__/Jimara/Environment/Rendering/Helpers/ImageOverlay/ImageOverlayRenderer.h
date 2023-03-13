#pragma once
#include "../../../../Graphics/Data/ShaderBinaries/ShaderLoader.h"


namespace Jimara {
	class JIMARA_API ImageOverlayRenderer : public virtual Object {
	public:
		static Reference<ImageOverlayRenderer> Create(
			Graphics::GraphicsDevice* device,
			Graphics::ShaderLoader* shaderLoader, 
			size_t maxInFlightCommandBuffers);

		virtual ~ImageOverlayRenderer();

		void SetSourceRegion(const Rect& region);

		void SetSource(Graphics::TextureSampler* sampler);

		void SetTargetRegion(const Rect& region);

		void SetTarget(Graphics::TextureView* target);

		void Execute(Graphics::Pipeline::CommandBufferInfo commandBuffer);

	private:
		const Reference<Object> m_data;

		inline ImageOverlayRenderer(Object* data) : m_data(data) {}

		struct Helpers;
	};
}

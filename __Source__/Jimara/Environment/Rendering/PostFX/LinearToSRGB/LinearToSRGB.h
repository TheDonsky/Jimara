#pragma once
#include "../../RenderStack.h"


namespace Jimara {
	class JIMARA_API LinearToSrgbKernel : public virtual RenderStack::Renderer {
	public:
		static Reference<LinearToSrgbKernel> Create(
			Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers);

		void Execute(
			Graphics::TextureView* source, Graphics::TextureView* result, const Graphics::InFlightBufferInfo& commandBuffer);

	private:
		inline LinearToSrgbKernel() {}

		struct Helpers;
	};
}

#pragma once
#include "../../RenderStack.h"


namespace Jimara {
	/// <summary>
	/// Simple kernel that can be used to translate images from Linear to SRGB color space
	/// <para/> Kernel also implements RenderStack::Renderer in case one wants to use this one inside the render loop.
	/// </summary>
	class JIMARA_API LinearToSrgbKernel : public virtual RenderStack::Renderer {
	public:
		/// <summary>
		/// Creates LinearToSrgbKernel instance
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="shaderLibrary"> Shader library </param>
		/// <param name="maxInFlightCommandBuffers"> In-flight command buffer count </param>
		/// <returns></returns>
		static Reference<LinearToSrgbKernel> Create(
			Graphics::GraphicsDevice* device, ShaderLibrary* shaderLibrary, size_t maxInFlightCommandBuffers);

		/// <summary>
		/// Translate image from Linear to SRGB color space
		/// </summary>
		/// <param name="source"> Source(linear) image </param>
		/// <param name="result"> Result(SRGB) image (can be the same as source) </param>
		/// <param name="commandBuffer"> In-flight command buffer </param>
		void Execute(
			Graphics::TextureView* source, Graphics::TextureView* result, const Graphics::InFlightBufferInfo& commandBuffer);

	private:
		// Constructor is private
		inline LinearToSrgbKernel() {}

		// Implementation is hidden behind this..
		struct Helpers;
	};
}

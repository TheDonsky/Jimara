#pragma once
#include "../../RenderStack.h"
#include "../../SimpleComputeKernel.h"


namespace Jimara {
	/// <summary>
	/// Tonemapper PostFX
	/// </summary>
	class JIMARA_API TonemapperKernel : public virtual Object {
	public:
		/// <summary>
		/// Creates tonemapper
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="shaderLoader"> Shader bytecode loader </param>
		/// <param name="maxInFlightCommandBuffers"> Maximal number of in-flight command buffers </param>
		/// <returns> New instance of a TonemapperKernel </returns>
		static Reference<TonemapperKernel> Create(
			Graphics::GraphicsDevice* device,
			Graphics::ShaderLoader* shaderLoader,
			size_t maxInFlightCommandBuffers);

		/// <summary> Virtual destructor </summary>
		virtual ~TonemapperKernel();

		/// <summary>
		/// Sets target texture
		/// </summary>
		/// <param name="target"> Target texture view </param>
		void SetTarget(Graphics::TextureView* target);

		/// <summary>
		/// Dispatches the underlying pipeline(s)
		/// </summary>
		/// <param name="commandBuffer"> In-flight command buffer </param>
		void Execute(const Graphics::InFlightBufferInfo& commandBuffer);


	private:
		// Compute kernel wrapper
		const Reference<SimpleComputeKernel> m_kernel;

		// Target texture binding
		const Reference<Graphics::ResourceBinding<Graphics::TextureView>> m_target;

		// Constructor is... private. For reasons!
		TonemapperKernel(SimpleComputeKernel* kernel, Graphics::ResourceBinding<Graphics::TextureView>* target);
	};
}

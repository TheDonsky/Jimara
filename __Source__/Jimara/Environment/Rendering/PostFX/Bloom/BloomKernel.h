#pragma once
#include "../../../../Graphics/Data/ShaderBinaries/ShaderLoader.h"


namespace Jimara {
	/// <summary>
	/// Bloom Post Processing effect
	/// </summary>
	class JIMARA_API BloomKernel : public virtual Object {
	public:
		/// <summary>
		/// Creates bloom kernel
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="shaderLoader"> Shader module loader </param>
		/// <param name="maxInFlightCommandBuffers"> Maximal number of simultanuous command buffers allowed </param>
		/// <returns></returns>
		static Reference<BloomKernel> Create(
			Graphics::GraphicsDevice* device, 
			Graphics::ShaderLoader* shaderLoader,
			size_t maxInFlightCommandBuffers);

		/// <summary> Virtual destructor </summary>
		virtual ~BloomKernel();

		/// <summary>
		/// Runs bloom post process
		/// </summary>
		/// <param name="source"> Source image that just got rendered (if null, kernel will not be executed) </param>
		/// <param name="destination"> Result image view (if null, source->TargetView() will be used instead) </param>
		/// <param name="commandBuffer"> Command buffer & in-flight index </param>
		void Execute(
			Graphics::TextureSampler* source,
			Graphics::TextureView* destination,
			Graphics::Pipeline::CommandBufferInfo commandBuffer);

	private:
		// Underlying data
		const Reference<Object> m_data;

		// Constructor needs to be private
		BloomKernel(
			Graphics::GraphicsDevice* device, 
			Graphics::Shader* downsample, Graphics::Shader* upsample, 
			size_t maxInFlightCommandBuffers);

		// Some private stuff is here
		struct Helpers;
	};
}

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
		/// Configures the bloom kernel settings
		/// </summary>
		/// <param name="spread"> "spread" amount for the upsample filter </param>
		void Configure(float spread);

		/// <summary>
		/// Sets source and destination textures
		/// </summary>
		/// <param name="source"> Source image (If nullptr, destination will be ignored; Prefferrably, wrapping mode should NOT be REPEAT) </param>
		/// <param name="destination"> Destination image (nullptr means the same as source->TargetTexture()) </param>
		void SetTextures(Graphics::TextureSampler* source, Graphics::TextureView* destination = nullptr);

		/// <summary>
		/// Runs bloom post process
		/// </summary>
		/// <param name="commandBuffer"> Command buffer & in-flight index </param>
		void Execute(Graphics::Pipeline::CommandBufferInfo commandBuffer);

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

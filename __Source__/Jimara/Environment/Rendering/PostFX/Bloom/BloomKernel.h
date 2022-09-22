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
		/// <param name="strength"> Bloom amount </param>
		/// <param name="size"> "size" for the upsample filter </param>
		void Configure(float strength, float size);

		/// <summary>
		/// Sets target texture
		/// </summary>
		/// <param name="image"> Non-bloomed image the effect should be directly applied on (copy to other texture to preserve original data; nullptr will clear internal images) </param>
		void SetTarget(Graphics::TextureSampler* image);

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

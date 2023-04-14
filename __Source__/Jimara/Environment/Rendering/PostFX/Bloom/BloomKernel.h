#pragma once
#include "../../../../Graphics/Data/ShaderBinaries/ShaderLoader.h"
#define BloomKernel_USE_NEW_PIPELINES


namespace Jimara {
	/// <summary>
	/// Bloom Post Processing effect
	/// <para/> Note: 
	/// <para/>		Implementation inspired by:
	/// <para/>			http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
	/// <para/>			https://www.froyok.fr/blog/2021-12-ue4-custom-bloom/
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
		/// <param name="threshold"> Minimal amount of pixel intensity for it to start blooming (negative values mean 'no thresholding') </param>
		/// <param name="thresholdSize"> Bloom will gradually fade in and out between intensities equal to threshold and (threshold + thresholdSize) </param>
		void Configure(float strength, float size, float threshold, float thresholdSize);

		/// <summary>
		/// Sets dirt texture for bloom overlay
		/// Note: Tiling 1 and offset 0 means "scale to fill"
		/// </summary>
		/// <param name="image"> Dirt texture to use (nullptr means no dirt) </param>
		/// <param name="strength"> Dirt bloom intensity </param>
		/// <param name="tiling"> Dirt texture tiling </param>
		/// <param name="offset"> Dirt texture offset </param>
		void SetDirtTexture(Graphics::TextureSampler* image, float strength = 1.0f, const Vector2& tiling = Vector2(1.0f), const Vector2& offset = Vector2(0.0f));

		/// <summary>
		/// Sets target texture
		/// </summary>
		/// <param name="image"> Non-bloomed image the effect should be directly applied on (copy to other texture to preserve original data; nullptr will clear internal images) </param>
		void SetTarget(Graphics::TextureSampler* image);

		/// <summary>
		/// Runs bloom post process
		/// </summary>
		/// <param name="commandBuffer"> Command buffer & in-flight index </param>
		void Execute(Graphics::InFlightBufferInfo commandBuffer);

	private:
		// Underlying data
		const Reference<Object> m_data;

		// Constructor needs to be private
		BloomKernel(
			Graphics::GraphicsDevice* device,
			Graphics::BindingPool* bindingPool,
			Graphics::ComputePipeline* threshold,
			Graphics::ComputePipeline* downsample,
			Graphics::ComputePipeline* upsample,
			Graphics::ComputePipeline* mix);

		// Some private stuff is here
		struct Helpers;
	};
}

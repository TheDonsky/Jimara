#pragma once
#include "../../../../Data/ShaderLibrary.h"


namespace Jimara {
	/// <summary>
	/// "Software Blit" pipeline that takes transparency into consideration
	/// </summary>
	class JIMARA_API ImageOverlayRenderer : public virtual Object {
	public:
		/// <summary>
		/// Creates an ImageOverlayRenderer object
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="shaderLibrary"> Shader library </param>
		/// <param name="maxInFlightCommandBuffers"> Maximal number of in-flight command buffers </param>
		/// <returns> New instance of an ImageOverlayRenderer pipelines </returns>
		static Reference<ImageOverlayRenderer> Create(
			Graphics::GraphicsDevice* device,
			ShaderLibrary* shaderLibrary, 
			size_t maxInFlightCommandBuffers);

		/// <summary> Virtual destructor </summary>
		virtual ~ImageOverlayRenderer();

		/// <summary>
		/// Sets source image rect (default to Rect(Vector2(0), Vector2(1)) if not set)
		/// </summary>
		/// <param name="region"> Region to use </param>
		void SetSourceRegion(const Rect& region);

		/// <summary>
		/// Sets source image
		/// </summary>
		/// <param name="sampler"> Source image sampler </param>
		void SetSource(Graphics::TextureSampler* sampler);

		/// <summary>
		/// Sets target image region (default to Rect(Vector2(0), Vector2(1)) if not set)
		/// </summary>
		/// <param name="region"> Region to blit to (end has to be greater than start in all dimensions) </param>
		void SetTargetRegion(const Rect& region);

		/// <summary>
		/// Sets target image
		/// </summary>
		/// <param name="target"> View to an image we're blitting to </param>
		void SetTarget(Graphics::TextureView* target);

		/// <summary>
		/// Executes blit pipeline
		/// </summary>
		/// <param name="commandBuffer"> Command buffer and index </param>
		void Execute(const Graphics::InFlightBufferInfo& commandBuffer);

	private:
		// Internals
		const Reference<Object> m_data;

		// Constructor can only be called via Create() call
		inline ImageOverlayRenderer(Object* data) : m_data(data) {}

		// Private stuff resides in here:
		struct Helpers;
	};
}

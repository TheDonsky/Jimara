#pragma once
#include "../LightingModel.h"
#include "../../SceneObjects/GraphicsObjectDescriptor.h"

#define DepthOnlyRenderer_USE_LIGHTING_MODEL_PIPELINES
#include "../LightingModelPipelines.h"


namespace Jimara {
	/// <summary>
	/// Renderer, that does a depth-only pass (useful for shadows)
	/// </summary>
	class JIMARA_API DepthOnlyRenderer : public virtual JobSystem::Job {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="viewport"> Viewport descriptor </param>
		/// <param name="layers"> Layers for object filtering </param>
		DepthOnlyRenderer(const ViewportDescriptor* viewport, LayerMask layers);

		/// <summary> Virtual destructor </summary>
		virtual ~DepthOnlyRenderer();

		/// <summary> Pixel format for target textures (other formats are not supported!) </summary>
		Graphics::Texture::PixelFormat TargetTextureFormat()const;

		/// <summary>
		/// Sets target texture to render to
		/// </summary>
		/// <param name="texture"> Texture to render to from next update onwards </param>
		void SetTargetTexture(Graphics::TextureView* depthTexture);
		
		/// <summary>
		/// Renders to target texture
		/// </summary>
		/// <param name="commandBufferInfo"> Command buffer an in-flight index </param>
		void Render(Graphics::Pipeline::CommandBufferInfo commandBufferInfo);

	protected:
		/// <summary> Invoked by job system to render what's needed </summary>
		virtual void Execute() override;

		/// <summary>
		/// Reports dependencies, if there are any
		/// </summary>
		/// <param name="addDependency"> Calling this will record dependency for given job </param>
		virtual void CollectDependencies(Callback<JobSystem::Job*> addDependency) override;

	private:
		// Viewport
		const Reference<const ViewportDescriptor> m_viewport;

		// LightingModelPipelines
		const Reference<LightingModelPipelines> m_lightingModelPipelines;

		// Environment descriptor
		const Reference<Graphics::ShaderResourceBindings::ShaderResourceBindingSet> m_environmentDescriptor;

		// Dynamic state
		SpinLock m_textureLock;
		Reference<Graphics::TextureView> m_targetTexture;
		Reference<Graphics::TextureView> m_frameBufferTexture;
		Reference<Graphics::FrameBuffer> m_frameBuffer;
		Reference<Graphics::Pipeline> m_environmentPipeline;
		Reference<LightingModelPipelines::Instance> m_pipelines;

		// Some private stuff resides in here
		struct Helpers;
	};
}

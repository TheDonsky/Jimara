#pragma once
#include "../LightingModel.h"
#define DepthOnlyRenderer_USE_GRAPHICS_OBJECT_PIPELINES
#include "../GraphicsObjectPipelines.h"


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
		/// <param name="graphicsObjectViewport"> 
		///		Most often, DepthOnlyRenderer is used for shadowmapping; 
		///		In this case, the geometry should be the same as in the camera view and this is the way to provide it's viewport.
		/// </param>
		DepthOnlyRenderer(const ViewportDescriptor* viewport, LayerMask layers, const ViewportDescriptor* graphicsObjectViewport = nullptr);

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
		void Render(Graphics::InFlightBufferInfo commandBufferInfo);

		/// <summary>
		///  Reports dependencies (basically, the same as JobSystem::Job::CollectDependencies, but public)
		/// </summary>
		/// <param name="addDependency"> Calling this will record dependency for given job </param>
		void GetDependencies(const Callback<JobSystem::Job*>& addDependency);

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

		// graphicsObjectViewport
		const Reference<const ViewportDescriptor> m_graphicsObjectViewport;

		// Graphics object descripotors
		const Reference<GraphicsObjectDescriptor::Set> m_graphicsObjectDescriptors;

		// Layer filter
		const LayerMask m_layers;

		// Viewport info cbuffer
		struct ViewportBuffer_t {
			alignas(16) Matrix4 view;
			alignas(16) Matrix4 projection;
		};
		const Graphics::BufferReference<ViewportBuffer_t> m_viewportBuffer;

		// Dynamic state
		SpinLock m_textureLock;
		Reference<Graphics::TextureView> m_targetTexture;
		Reference<Graphics::TextureView> m_frameBufferTexture;
		Reference<Graphics::FrameBuffer> m_frameBuffer;
		Reference<Graphics::BindingPool> m_bindingPool;
		Reference<GraphicsObjectPipelines> m_pipelines;
		Stacktor<Reference<Graphics::BindingSet>, 4> m_bindingSets;

		// Some private stuff resides in here
		struct Helpers;
	};
}

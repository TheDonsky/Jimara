#pragma once
#include "../Utilities/GraphicsObjectPipelines.h"
#include "../../../GraphicsSimulation/GraphicsSimulation.h"


namespace Jimara {
	/// <summary>
	/// Renders dual-paraboloid depth maps (useful for point light shadows, for example)
	/// </summary>
	class JIMARA_API DualParaboloidDepthRenderer : public virtual JobSystem::Job {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <param name="layers"> Layers for object filtering </param>
		/// <param name="rendererFrustrum"> Renderer frustrum descriptor for RendererFrustrumDescriptor::ViewportFrustrumDescriptor (mainly to match the LOD-s from viewport) </param>
		/// <param name="frustrumFlags"> Flags for the underlying RendererFrustrumDescriptor </param>
		DualParaboloidDepthRenderer(Scene::LogicContext* context, LayerMask layers, const RendererFrustrumDescriptor* rendererFrustrum, RendererFrustrumFlags frustrumFlags);

		/// <summary> Virtual destructor </summary>
		virtual ~DualParaboloidDepthRenderer();

		/// <summary>
		/// Configures position and clipping planes
		/// </summary>
		/// <param name="position"> World space position </param>
		/// <param name="closePlane"> Close clipping plane </param>
		/// <param name="farPlane"> Far clipping plane </param>
		void Configure(const Vector3& position, float closePlane = 0.001f, float farPlane = 1000.0f);

		/// <summary> Pixel format for target textures (other formats are not supported!) </summary>
		Graphics::Texture::PixelFormat TargetTextureFormat()const;

		/// <summary>
		/// Sets target texture to render to
		/// <para/> Note: Roughly describe what kind of a texture we expect here...
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
		// Scene context
		const Reference<Scene::LogicContext> m_context;

		// Layer filter
		const LayerMask m_layers;

		// Graphics object descripotors
		const Reference<GraphicsObjectDescriptor::Set> m_graphicsObjectDescriptors;

		// Environment Constrant buffer and pipeline
		const Reference<Graphics::Buffer> m_constantBufferFront;
		const Reference<Graphics::Buffer> m_constantBufferBack;

		// Settings:
		struct FrustrumSettings : public virtual RendererFrustrumDescriptor {
			mutable SpinLock lock;
			Vector3 position = Vector3(0.0f);
			float closePlane = 0.001f;
			float farPlane = 1000.0f;
			const Reference<const RendererFrustrumDescriptor> m_viewportFrustrum;

			FrustrumSettings(const RendererFrustrumDescriptor* viewportFrustrum, RendererFrustrumFlags frustrumFlags);
			virtual ~FrustrumSettings();
			virtual Matrix4 FrustrumTransform()const override;
			virtual Vector3 EyePosition()const override;
			virtual const RendererFrustrumDescriptor* ViewportFrustrumDescriptor()const override;
		};
		const Reference<FrustrumSettings> m_settings;

		// Graphics simulation jobs
		const Reference<GraphicsSimulation::JobDependencies> m_graphicsSimulation;

		// Dynamic state
		SpinLock m_textureLock;
		Reference<Graphics::TextureView> m_targetTexture;
		Reference<Graphics::TextureView> m_frameBufferTexture;
		Reference<Graphics::FrameBuffer> m_frameBuffer;
		Reference<Graphics::BindingPool> m_bindingPool;
		Reference<GraphicsObjectPipelines> m_pipelines;
		Stacktor<Reference<Graphics::BindingSet>, 4u> m_bindingSetsFront;
		Stacktor<Reference<Graphics::BindingSet>, 4u> m_bindingSetsBack;

		// Some private stuff resides in here
		struct Helpers;
	};
}

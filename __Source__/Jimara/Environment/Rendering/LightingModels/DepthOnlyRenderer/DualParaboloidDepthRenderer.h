#pragma once
#include "../LightingModelPipelines.h"



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
		DualParaboloidDepthRenderer(Scene::LogicContext* context, LayerMask layers);

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
		// Scene context
		const Reference<Scene::LogicContext> m_context;

		// LightingModelPipelines
		const Reference<LightingModelPipelines> m_lightingModelPipelines;

		// Environment Constrant buffer and pipeline
		const Reference<Graphics::Buffer> m_constantBufferFront;
		const Reference<Graphics::Buffer> m_constantBufferBack;
		Reference<Graphics::Pipeline> m_environmentPipelineFront;
		Reference<Graphics::Pipeline> m_environmentPipelineBack;

		// Settings:
		struct {
			SpinLock lock;
			Vector3 position = Vector3(0.0f);
			float closePlane = 0.001f;
			float farPlane = 1000.0f;
		} m_settings;

		// Dynamic state
		SpinLock m_textureLock;
		Reference<Graphics::TextureView> m_targetTexture;
		Reference<Graphics::TextureView> m_frameBufferTexture;
		Reference<Graphics::FrameBuffer> m_frameBuffer;
		Reference<LightingModelPipelines::Instance> m_pipelines;

		// Some private stuff resides in here
		struct Helpers;
	};
}

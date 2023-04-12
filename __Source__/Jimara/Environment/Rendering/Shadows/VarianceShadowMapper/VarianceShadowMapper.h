#pragma once
#include "../../../Scene/Scene.h"



namespace Jimara {
	/// <summary>
	/// Generates a variance shadow map from a clip-space depth map
	/// </summary>
	class JIMARA_API VarianceShadowMapper : public virtual JobSystem::Job {
	public:
		/// <summary>
		/// Creates a new instance of VarianceShadowMapper
		/// </summary>
		/// <param name="context"> Scene context (job will be able to be executed as a part of the render job system) </param>
		/// <returns> New VarianceShadowMapper </returns>
		static Reference<VarianceShadowMapper> Create(SceneContext* context);

		/// <summary> Virtual destructor </summary>
		~VarianceShadowMapper();

		/// <summary>
		/// Sets main configuration variables
		/// </summary>
		/// <param name="closePlane"> Close clipping plane </param>
		/// <param name="farPlane"> Far clipping plane </param>
		/// <param name="softness"> Higher the value, less sharp the image will appear </param>
		/// <param name="filterSize"> Size of the gaussian blur filter </param>
		/// <param name="linearDepth"> True means that the depth map is linear; false means clip-space </param>
		void Configure(float closePlane, float farPlane, float softness = 1.0f, uint32_t filterSize = 5u, bool linearDepth = false);

		/// <summary>
		/// Sets 'source' texture
		/// </summary>
		/// <param name="depthBuffer"> Depth texture (close and far clipping planes should match the configuration above) </param>
		/// <param name="fp32Variance"> If true, variance map will be R32G32_SFLOAT, otherwise R16G16_SFLOAT </param>
		/// <returns> Result texture sampler </returns>
		Reference<Graphics::TextureSampler> SetDepthTexture(Graphics::TextureSampler* depthBuffer, bool fp32Variance = false);

		/// <summary>
		/// Variance map that the job will be writing to
		/// </summary>
		/// <returns> Result texture sampler </returns>
		Reference<Graphics::TextureSampler> VarianceMap()const;

		/// <summary>
		/// Generates variance shadow map and stores it in VarianceMap()
		/// </summary>
		/// <param name="commandBufferInfo"> Command buffer and in-flight index (empty will mean GetWorkerThreadCommandBuffer()) </param>
		void GenerateVarianceMap(Graphics::InFlightBufferInfo commandBufferInfo);

	protected:
		/// <summary> Invokes GenerateVarianceMap() with GetWorkerThreadCommandBuffer() </summary>
		virtual void Execute() override;

		/// <summary>
		/// Does nothing
		/// </summary>
		/// <param name=""> Ignored </param>
		inline virtual void CollectDependencies(Callback<Job*>) override {}

	private:
		// Owner context
		const Reference<SceneContext> m_context;

		const Reference<Graphics::Experimental::ComputePipeline> m_vsmPipeline;
		const Reference<Graphics::BindingSet> m_bindingSet;
		
		struct Params {
			alignas(4) float closePlane = 0.01f;
			alignas(4) float farPlane = 1000.0f;
			alignas(4) uint32_t filterSize = 1;
			alignas(4) uint32_t linearDepth = 0;
		};
		mutable SpinLock m_lock;
		Params m_params;
		float m_softness = 0.001f;
		const Graphics::BufferReference<Params> m_paramsBuffer;
		const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_blurFilter;
		const Reference<Graphics::ResourceBinding<Graphics::TextureSampler>> m_depthBuffer;
		const Reference<Graphics::ResourceBinding<Graphics::TextureView>> m_varianceMap;
		Reference<Graphics::TextureSampler> m_varianceSampler;

		// Private constructor
		VarianceShadowMapper(
			SceneContext* context,
			Graphics::Experimental::ComputePipeline* vsmPipeline,
			Graphics::BindingSet* bindingSet,
			Graphics::Buffer* params,
			Graphics::ResourceBinding<Graphics::ArrayBuffer>* blurFilter,
			Graphics::ResourceBinding<Graphics::TextureSampler>* depthBuffer,
			Graphics::ResourceBinding<Graphics::TextureView>* varianceMap);
	};
}

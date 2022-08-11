#pragma once
#include "../../../Scene/Scene.h"



namespace Jimara {
	/// <summary>
	/// Generates a variance shadow map from a clip-space depth map
	/// </summary>
	class JIMARA_API VarianceShadowMapper : public virtual JobSystem::Job {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="context"> Scene context (job will be able to be executed as a part of the render job system) </param>
		VarianceShadowMapper(SceneContext* context);

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
		/// <param name="clipSpaceDepth"> Clip-space depth texture (close and far clipping planes should match the configuration above) </param>
		/// <returns> Result texture sampler </returns>
		Reference<Graphics::TextureSampler> SetDepthTexture(Graphics::TextureSampler* clipSpaceDepth);

		/// <summary>
		/// Variance map that the job will be writing to
		/// </summary>
		/// <returns> Result texture sampler </returns>
		Reference<Graphics::TextureSampler> VarianceMap()const;

		/// <summary>
		/// Generates variance shadow map and stores it in VarianceMap()
		/// </summary>
		/// <param name="commandBufferInfo"> Command buffer and in-flight index (empty will mean GetWorkerThreadCommandBuffer()) </param>
		void GenerateVarianceMap(Graphics::Pipeline::CommandBufferInfo commandBufferInfo);

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

		// Underlying pipeline descriptor
		const Reference<Graphics::ComputePipeline::Descriptor> m_pipelineDescriptor;

		// Underlying pipeline
		Reference<Graphics::ComputePipeline> m_computePipeline;

		// Some private helper functionality
		struct Helpers;
	};
}

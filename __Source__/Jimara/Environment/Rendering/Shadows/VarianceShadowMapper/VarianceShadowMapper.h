#pragma once
#include "../../../Scene/Scene.h"



namespace Jimara {
	class JIMARA_API VarianceShadowMapper : public virtual JobSystem::Job {
	public:
		VarianceShadowMapper(SceneContext* context);

		~VarianceShadowMapper();

		void Configure(float closePlane, float farPlane, float softness = 1.0f, uint32_t filterSize = 5u);

		Reference<Graphics::TextureSampler> SetDepthTexture(Graphics::TextureSampler* clipSpaceDepth);

		Reference<Graphics::TextureSampler> VarianceMap()const;

		void GenerateVarianceMap(Graphics::Pipeline::CommandBufferInfo commandBufferInfo);

	protected:
		virtual void Execute() override;

		inline virtual void CollectDependencies(Callback<Job*>) override {}

	private:
		const Reference<SceneContext> m_context;
		const Reference<Graphics::ComputePipeline::Descriptor> m_pipelineDescriptor;
		Reference<Graphics::ComputePipeline> m_computePipeline;

		struct Helpers;
	};
}

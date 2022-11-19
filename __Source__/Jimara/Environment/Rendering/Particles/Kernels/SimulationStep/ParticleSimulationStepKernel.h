#pragma once
#include "../../ParticleKernel.h"


namespace Jimara {
	class JIMARA_API ParticleSimulationStepKernel : public virtual ParticleKernel {
	public:
		enum class TimeMode : uint32_t {
			NO_TIME = 0u,
			UNSCALED_DELTA_TIME = 1u,
			SCALED_DELTA_TIME = 2u,
			PHYSICS_DELTA_TIME = 3u
		};

		class Task : public virtual ParticleKernel::Task {
		public:
			Task(SceneContext* context);

			virtual ~Task();

			void SetBuffers(ParticleBuffers* buffers);

			void SetTimeScale(float timeScale);

			void SetTimeMode(TimeMode timeMode);

			virtual void Synchronize()override;

		private:
			SpinLock m_bufferLock;
			Reference<ParticleBuffers> m_buffers;
			std::atomic<float> m_timeScale = 1.0f;
			std::atomic<uint32_t> m_timeMode = static_cast<uint32_t>(TimeMode::SCALED_DELTA_TIME);

			Reference<ParticleBuffers> m_lastBuffers;
			Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_particleStateBuffer;
		};
		
		virtual ~ParticleSimulationStepKernel();

		virtual Reference<ParticleKernel::Instance> CreateInstance(SceneContext* context)const override;

	private:
		ParticleSimulationStepKernel();

		struct Helpers;
	};
}

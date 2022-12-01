#include "ParticleState.h"
#include "../../GraphicsSimulation/CombinedGraphicsSimulationKernel.h"


namespace Jimara {
	namespace {
		struct ParticleState_InitializationKernel : public virtual ParticleBuffers::AllocationKernel {
		private:
			struct TaskSettings {
				alignas(4) uint32_t liveParticleCountBufferId = 0u;		// Bytes [0 - 4)
				alignas(4) uint32_t particleIndirectionBufferId = 0u;	// Bytes [4 - 8)
				alignas(4) uint32_t stateBufferId = 0u;					// Bytes [8 - 12)
				alignas(4) uint32_t particleBudget = 0u;				// Bytes [12 - 16)
				alignas(4) uint32_t taskThreadCount = 0u;				// Bytes [16 - 20)
			};

			class AllocationTask : public virtual ParticleBuffers::AllocationTask {
			private:
				const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_stateBuffer;
				const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_indirectionBuffer;
				const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_liveParticleCount;
				TaskSettings m_settings = {};

			public:
				inline AllocationTask(
					SceneContext* context,
					Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* buffer,
					Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* indirectionBuffer,
					Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* liveParticleCount) 
					: GraphicsSimulation::Task(Instance(), context)
					, m_stateBuffer(buffer)
					, m_indirectionBuffer(indirectionBuffer)
					, m_liveParticleCount(liveParticleCount) {
					m_settings.liveParticleCountBufferId = m_liveParticleCount->Index();
					m_settings.particleIndirectionBufferId = m_indirectionBuffer->Index();
					m_settings.stateBufferId = m_stateBuffer->Index();
					m_settings.particleBudget = static_cast<uint32_t>(m_stateBuffer->BoundObject()->ObjectCount());
					SetSettings(m_settings);
				}

				inline virtual ~AllocationTask() {}

				inline virtual void Synchronize() override {
					m_settings.taskThreadCount = SpawnedParticleCount();
					SetSettings(m_settings);
				}
			};

		public:
			inline ParticleState_InitializationKernel() : GraphicsSimulation::Kernel(sizeof(TaskSettings)) {}
			inline virtual ~ParticleState_InitializationKernel() {}

			inline static const ParticleState_InitializationKernel* Instance() {
				static const ParticleState_InitializationKernel instance;
				return &instance;
			}

			inline virtual Reference<GraphicsSimulation::KernelInstance> CreateInstance(SceneContext* context)const override {
				static const Graphics::ShaderClass INITIALIZATION_SHADER("Jimara/Environment/Rendering/Particles/ParticleState_AllocationKernel");
				return CombinedGraphicsSimulationKernel<TaskSettings>::Create(
					context, &INITIALIZATION_SHADER, Graphics::ShaderResourceBindings::ShaderBindingDescription());
			}

			inline virtual Reference<ParticleBuffers::AllocationTask> CreateTask(
				SceneContext* context,
				uint32_t particleBudget,
				Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* buffer,
				Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* indirectionBuffer,
				Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* liveParticleCount)const override {
				if (context == nullptr) return nullptr;
				auto error = [&](auto... message) -> Reference<ParticleBuffers::AllocationTask> {
					context->Log()->Error("ParticleState_InitializationKernel::CreateTask - ", message...); return nullptr; 
				};
				if (buffer == nullptr) return error("State buffer not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				else if (indirectionBuffer == nullptr) return error("Indirection buffer not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				else if (liveParticleCount == nullptr) return error("Live particle buffer not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				else if (buffer->BoundObject()->ObjectCount() != particleBudget || buffer->BoundObject()->ObjectSize() != sizeof(ParticleState))
					return error("buffer expected to be an ParticleState buffer with particleBudget element count! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				else if (indirectionBuffer->BoundObject()->ObjectCount() != particleBudget || indirectionBuffer->BoundObject()->ObjectSize() != sizeof(uint32_t))
					return error("indirectionBuffer expected to be an uint32_t buffer with particleBudget element count! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				else if (liveParticleCount->BoundObject()->ObjectCount() != 1u || liveParticleCount->BoundObject()->ObjectSize() != sizeof(uint32_t))
					return error("liveParticleCount expected to be an uint32_t buffer with one element! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				else return Object::Instantiate<AllocationTask>(context, buffer, indirectionBuffer, liveParticleCount);
			}
		};
	}

	const ParticleBuffers::BufferId* ParticleState::BufferId() {
		static const Reference<const ParticleBuffers::BufferId> BUFFER_ID =
			ParticleBuffers::BufferId::Create<ParticleState>("ParticleState", ParticleState_InitializationKernel::Instance());
		return BUFFER_ID;
	}
}

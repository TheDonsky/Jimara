#include "ParticleState.h"
#include "../../GraphicsSimulation/CombinedGraphicsSimulationKernel.h"


namespace Jimara {
	namespace {
		struct ParticleState_InitializationKernel : public virtual ParticleBuffers::AllocationKernel {
		private:
			struct TaskSettings {
				alignas(16) Vector3 position = Vector3(0.0f);			// Bytes [0 - 12)
				alignas(4) uint32_t liveParticleCountBufferId = 0u;		// Bytes [12 - 16)
				alignas(16) Vector3 eulerAngles = Vector3(0.0f);		// Bytes [16 - 28)
				alignas(4) uint32_t particleIndirectionBufferId = 0u;	// Bytes [28 - 32)
				alignas(16) Vector3 scale = Vector3(1.0f);				// Bytes [32 - 44)
				alignas(4) uint32_t stateBufferId = 0u;					// Bytes [44 - 48)
				alignas(4) uint32_t particleBudget = 0u;				// Bytes [48 - 52)
				alignas(4) uint32_t taskThreadCount = 0u;				// Bytes [52 - 56)
				alignas(4) uint32_t pad_0 = 0u, pad1 = 0u;				// Bytes [56 - 64)
			};
			static_assert(sizeof(TaskSettings) == 64);

			class AllocationTask : public virtual ParticleBuffers::AllocationTask {
			private:
				const Reference<const ParticleSystemInfo> m_systemInfo;
				const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_stateBuffer;
				const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_indirectionBuffer;
				const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_liveParticleCount;
				TaskSettings m_settings = {};

			public:
				inline AllocationTask(
					const ParticleSystemInfo* systemInfo,
					Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* buffer,
					Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* indirectionBuffer,
					Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* liveParticleCount) 
					: GraphicsSimulation::Task(Instance(), systemInfo->Context())
					, m_systemInfo(systemInfo)
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
					if (m_systemInfo->SimulationSpace() == ParticleSystemInfo::SimulationMode::WORLD_SPACE) {
						const Matrix4 transform = m_systemInfo->WorldTransform();
						const Vector3 scale = Vector3(
							Math::Magnitude((Vector3)transform[0]),
							Math::Magnitude((Vector3)transform[1]),
							Math::Magnitude((Vector3)transform[2]));
						m_settings.position = transform[3];
						m_settings.eulerAngles = Math::EulerAnglesFromMatrix([&]() -> Matrix4 {
							Matrix4 mat = Math::Identity();
							if (scale.x > 0.0f) mat[0] = Vector4(((Vector3)transform[0]) / scale.x, 0.0f);
							if (scale.y > 0.0f) mat[1] = Vector4(((Vector3)transform[1]) / scale.y, 0.0f);
							if (scale.z > 0.0f) mat[2] = Vector4(((Vector3)transform[2]) / scale.z, 0.0f);
							if (scale.x > 0.0f) {
								if (scale.y > 0.0f) mat[2] = Vector4(Math::Cross((Vector3)mat[0], (Vector3)mat[1]), 0.0f);
								else if (scale.z > 0.0f) mat[1] = Vector4(Math::Cross((Vector3)mat[2], (Vector3)mat[0]), 0.0f);
							}
							else mat[0] = Vector4(Math::Cross((Vector3)mat[1], (Vector3)mat[2]), 0.0f);
							mat[3] = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
							return mat;
							}());
						m_settings.scale = scale;
					}
					else {
						m_settings.position = Vector3(0.0f);
						m_settings.eulerAngles = Vector3(0.0f);
						m_settings.scale = Vector3(1.0f);
					}

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
				const ParticleSystemInfo* systemInfo,
				uint32_t particleBudget,
				Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* buffer,
				Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* indirectionBuffer,
				Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* liveParticleCount)const override {
				if (systemInfo == nullptr) return nullptr;
				auto error = [&](auto... message) -> Reference<ParticleBuffers::AllocationTask> {
					systemInfo->Context()->Log()->Error("ParticleState_InitializationKernel::CreateTask - ", message...); return nullptr;
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
				else return Object::Instantiate<AllocationTask>(systemInfo, buffer, indirectionBuffer, liveParticleCount);
			}
		};
	}

	const ParticleBuffers::BufferId* ParticleState::BufferId() {
		static const Reference<const ParticleBuffers::BufferId> BUFFER_ID =
			ParticleBuffers::BufferId::Create<ParticleState>("ParticleState", ParticleState_InitializationKernel::Instance());
		return BUFFER_ID;
	}
}

#include "PlaceInSphere.h"
#include "../../../ParticleState.h"
#include "../../../../Algorithms/Random/GraphicsRNG.h"
#include "../../../../../GraphicsSimulation/CombinedGraphicsSimulationKernel.h"
#include "../../../../../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	namespace InitializationKernels {
		class PlaceOnSphere::Kernel : public virtual GraphicsSimulation::Kernel {
		private:
			class KernelInstance : public virtual GraphicsSimulation::KernelInstance {
			private:
				const Reference<SceneContext> m_context;
				const Reference<GraphicsRNG> m_graphicsRNG;
				const Reference<GraphicsSimulation::KernelInstance> m_combinedKernel;
				const Reference<Graphics::ShaderResourceBindings::StructuredBufferBinding> m_rngBufferBinding;

			public:
				inline KernelInstance(
					SceneContext* context,
					GraphicsRNG* graphicsRNG,
					GraphicsSimulation::KernelInstance* combinedKernel,
					Graphics::ShaderResourceBindings::StructuredBufferBinding* bufferBinding)
					: m_context(context)
					, m_graphicsRNG(graphicsRNG)
					, m_combinedKernel(combinedKernel)
					, m_rngBufferBinding(bufferBinding) {}

				inline virtual ~KernelInstance() {}

				inline virtual void Execute(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, const Task* const* tasks, size_t taskCount) final override {
					size_t totalElementCount = 0u;
					{
						const Task* const* ptr = tasks;
						const Task* const* const end = ptr + taskCount;
						while (ptr < end) {
							totalElementCount += (*ptr)->GetSettings<SimulationTaskSettings>().taskThreadCount;
							ptr++;
						}
					}
					if (m_rngBufferBinding->BoundObject() == nullptr || m_rngBufferBinding->BoundObject()->ObjectCount() < totalElementCount) {
						m_rngBufferBinding->BoundObject() = m_graphicsRNG->GetBuffer(totalElementCount);
						if (m_rngBufferBinding->BoundObject() == nullptr) {
							m_context->Log()->Error(
								"PlaceOnSphere::Kernel::KernelInstance::Execute - Failed to retrieve Graphics RNG buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return;
						}
					}
					m_combinedKernel->Execute(commandBufferInfo, tasks, taskCount);
				}
			};

		public:
			inline Kernel() : GraphicsSimulation::Kernel(sizeof(SimulationTaskSettings)) {}
			inline virtual ~Kernel() {}
			static const Kernel* Instance() {
				static const Kernel instance;
				return &instance;
			}
			inline virtual Reference<GraphicsSimulation::KernelInstance> CreateInstance(SceneContext* context)const final override {
				if (context == nullptr) return nullptr;
				const Reference<GraphicsRNG> graphicsRNG = GraphicsRNG::GetShared(context);
				if (graphicsRNG == nullptr) {
					context->Log()->Error("PlaceOnSphere::Kernel::CreateInstance - Failed to get GraphicsRNG! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
				}
				static const Graphics::ShaderClass SHADER_CLASS("Jimara/Environment/Rendering/Particles/Kernels/Initialization/PlaceInSphere/PlaceInSphere");
				Graphics::ShaderResourceBindings::ShaderBindingDescription desc;
				const Reference<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding> binding =
					Object::Instantiate<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding>("graphicsRNG");
				const Graphics::ShaderResourceBindings::NamedStructuredBufferBinding* const bindingPtr = binding;
				desc.structuredBufferBindings = &bindingPtr;
				desc.structuredBufferBindingCount = 1u;
				const Reference<GraphicsSimulation::KernelInstance> combinedKernel = 
					CombinedGraphicsSimulationKernel<SimulationTaskSettings>::Create(context, &SHADER_CLASS, desc);
				if (combinedKernel == nullptr) {
					context->Log()->Error(
						"PlaceOnSphere::Kernel::CreateInstance - Failed to create CombinedGraphicsSimulationKernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
				}
				return Object::Instantiate<KernelInstance>(context, graphicsRNG, combinedKernel, binding);
			}
		};

		PlaceOnSphere::PlaceOnSphere(SceneContext* context)
			: GraphicsSimulation::Task(Kernel::Instance(), context) {}

		PlaceOnSphere::~PlaceOnSphere() {}

		void PlaceOnSphere::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				JIMARA_SERIALIZE_FIELD(m_simulationSettings.radius, "Radius", "Radius of the spawn area");
			};
		}

		void PlaceOnSphere::SetBuffers(
			Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* indirectionBuffer,
			Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* liveParticleCountBuffer,
			const ParticleBuffers::BufferSearchFn& findBuffer) {
			const Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* stateBuffer = findBuffer(ParticleState::BufferId());
			if (indirectionBuffer == nullptr || liveParticleCountBuffer == nullptr || stateBuffer == nullptr) {
				m_simulationSettings.liveParticleCountBufferId = 0u;
				m_simulationSettings.particleIndirectionBufferId = 0u;
				m_simulationSettings.stateBufferId = 0u;
				m_simulationSettings.particleBudget = 0u;
			}
			else {
				m_simulationSettings.liveParticleCountBufferId = liveParticleCountBuffer->Index();
				m_simulationSettings.particleIndirectionBufferId = indirectionBuffer->Index();
				m_simulationSettings.stateBufferId = stateBuffer->Index();
				m_simulationSettings.particleBudget = ParticleBudget();
			}
		}

		void PlaceOnSphere::UpdateSettings() {
			m_simulationSettings.taskThreadCount = SpawnedParticleCount();
			SetSettings(m_simulationSettings);
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<InitializationKernels::PlaceOnSphere>(const Callback<const Object*>& report) {
		static const Reference<const Object> factory = ParticleInitializationTask::Factory::Of<InitializationKernels::PlaceOnSphere>();
		report(factory);
	}
}

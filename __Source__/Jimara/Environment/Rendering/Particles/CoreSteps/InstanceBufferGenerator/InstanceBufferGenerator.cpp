#include "InstanceBufferGenerator.h"
#include "../../ParticleState.h"
#include "../../../Culling/FrustrumAABB/FrustrumAABBCulling.h"
#include "../../../../GraphicsSimulation/CombinedGraphicsSimulationKernel.h"


namespace Jimara {
	struct ParticleInstanceBufferGenerator::Helpers {
		/// <summary> When set, the rotation of the particle system will not be transfered to the particles </summary>
		static const constexpr uint32_t INDEPENDENT_PARTICLE_ROTATION = 1u;

		/// <summary> Tells the shader to care about viewport matrix </summary>
		static const constexpr uint32_t FACE_TOWARDS_VIEWPORT = 2u;

		/// <summary> Set when the viewport is a shadowmapper </summary>
		static const constexpr uint32_t VIEWPORT_IS_A_SHADOWMAPPER = 4u;

		/// <summary>
		/// Settings for a task
		/// </summary>
		struct JIMARA_API TaskSettings {
			/// <summary> 
			/// World matrix of the particle system, if the simulation runs in local space, Math::Identity() otherwise;
			/// Same, but multiplied by a viewport-facing rotation if we have camera-facing quads instead of meshes.
			/// </summary>
			alignas(16) Matrix4 baseTransform = Math::Identity();	// Bytes [0 - 64)

			/// <summary> Viewport 'right' direction </summary>
			alignas(16) Vector3 viewportRight = Math::Right();		// Bytes [64 - 76)

			/// <summary> Indirection/Index Wrangle bindless buffer id </summary>
			alignas(4) uint32_t particleIndirectionBufferId = 0u;	// Bytes [76 - 80)

			/// <summary> Viewport 'up' direction </summary>
			alignas(16) Vector3 viewportUp = Math::Up();			// Bytes [80 - 92)

			/// <summary> Bindless buffer id for ParticleState </summary>
			alignas(4) uint32_t particleStateBufferId = 0u;			// Bytes [92 - 96)

			/// <summary> Bindless buffer id for the resulting Matrix4 instance buffer </summary>
			alignas(4) uint32_t instanceBufferId = 0u;				// Bytes [96 - 100)

			/// <summary> Index of the first particle's instance within the instanceBuffer </summary>
			alignas(4) uint32_t instanceStartId = 0u;				// Bytes [100 - 104)

			/// <summary> Number of particles within the particle system (name is important; ) </summary>
			alignas(4) uint32_t taskThreadCount = 0u;				// Bytes [104 - 108)

			/// <summary> Bindless buffer id for the 'Live Particle Count' buffer </summary>
			alignas(4) uint32_t liveParticleCountBufferId = 0u;		// Bytes [108 - 112)

			/// <summary> Bindless buffer id for the 'Indirect draw buffer' </summary>
			alignas(4) uint32_t indirectDrawBufferId = 0u;			// Bytes [112 - 116)

			/// <summary> Index of the particle system within the Indirect draw buffer </summary>
			alignas(4) uint32_t indirectCommandIndex = 0u;			// Bytes [116 - 120)

			/// <summary> Particle flags like independent rotation and inset </summary>
			alignas(4) uint32_t flags = 0u;							// Bytes [120 - 124)

			/// <summary> Object index (without culling) </summary>
			alignas(4) uint32_t objectIndex = 0u;					// Bytes [124 - 128)
		};
		static_assert(sizeof(TaskSettings) == 128);

		class Kernel : public virtual GraphicsSimulation::Kernel {
		public:
			inline Kernel() : GraphicsSimulation::Kernel(sizeof(TaskSettings)) {}

			inline virtual ~Kernel() {}

			static Kernel* Instance() {
				static Kernel instance;
				return &instance;
			}

			inline virtual Reference<GraphicsSimulation::KernelInstance> CreateInstance(SceneContext* context)const override {
				if (context == nullptr) return nullptr;
				static const constexpr std::string_view SHADER_PATH(
					"Jimara/Environment/Rendering/Particles/CoreSteps/InstanceBufferGenerator/InstanceBufferGenerator_Kernel.comp");
				Reference<GraphicsSimulation::KernelInstance> kernel = CombinedGraphicsSimulationKernel<TaskSettings>::Create(context, SHADER_PATH, {});
				if (kernel == nullptr) {
					context->Log()->Error("ParticleInstanceBufferGenerator::Helpers::Kernel::CreateInstance - Failed to create combined kernel instance! ",
						"[File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
				}
				else return Object::Instantiate<KernelInstance>(context, kernel);
			}
		};

		class KernelInstance : public virtual GraphicsSimulation::KernelInstance {
		private:
			const Reference<SceneContext> m_context;
			const Reference<GraphicsSimulation::KernelInstance> m_combinedKernel;
			std::vector<const Task*> m_tasks;

		public:
			inline KernelInstance(SceneContext* context, GraphicsSimulation::KernelInstance* combinedKernel) 
				: m_context(context), m_combinedKernel(combinedKernel) {
				assert(m_context != nullptr);
				assert(m_combinedKernel != nullptr);
			}

			inline virtual ~KernelInstance() {}

			virtual void Execute(Graphics::InFlightBufferInfo commandBufferInfo, const Task* const* tasks, size_t taskCount) final override {
				m_tasks.clear();
				const Task* const* const end = tasks + taskCount;

				// Mark task invisible:
				for (const Task* const* ptr = tasks; ptr < end; ptr++) {
					const ParticleInstanceBufferGenerator* task = dynamic_cast<const ParticleInstanceBufferGenerator*>(*ptr);
					if (task == nullptr)
						continue;
					task->m_wasVisible = false;
				}

				// Update subtasks:
				for (const Task* const* ptr = tasks; ptr < end; ptr++) {
					const ParticleInstanceBufferGenerator* task = dynamic_cast<const ParticleInstanceBufferGenerator*>(*ptr);
					if (task == nullptr) {
						m_context->Log()->Warning(
							"ParticleInstanceBufferGenerator::Helpers::KernelInstance::Execute - Got unsupported task! ",
							"[File:", __FILE__, "; Line: ", __LINE__, "]");
						continue;
					}
					else {
						std::unique_lock<SpinLock> lock(task->m_lock);
						if (task->m_buffers == nullptr) continue;

						const Matrix4 baseTransform = task->m_baseTransform;
						const uint32_t independentParticleRotation = task->m_independentParticleRotation ? INDEPENDENT_PARTICLE_ROTATION : 0u;

						// Extract common settings:
						TaskSettings settings = {};
						{
							settings.particleIndirectionBufferId = task->m_particleIndirectionBuffer->Index();
							settings.particleStateBufferId = task->m_particleStateBuffer->Index();
							settings.liveParticleCountBufferId = task->m_liveParticleCountBuffer->Index();
							settings.taskThreadCount = static_cast<uint32_t>(task->m_buffers->ParticleBudget());
							settings.instanceStartId = static_cast<uint32_t>(task->m_instanceStartIndex);
							settings.objectIndex = static_cast<uint32_t>(task->m_objectIndex);
						}

						const ViewportTask* const subtaskEndPtr = task->m_viewTasks.Data() + task->m_viewTasks.Size();
						for (const ViewportTask* subtaskPtr = task->m_viewTasks.Data(); subtaskPtr < subtaskEndPtr; subtaskPtr++) {
							// Discard subtask if target buffer bindings are missing:
							if (subtaskPtr->transformBuffer == nullptr || 
								subtaskPtr->indirectDrawBuffer == nullptr ||
								subtaskPtr->indirectDrawCount == nullptr) {
								continue;
							}

							// Update transform:
							settings.baseTransform = baseTransform;
							settings.flags = independentParticleRotation;
							if (subtaskPtr->viewport != nullptr) {
								// Set view matrix:
								const Matrix4 viewMatrix = subtaskPtr->viewport->ViewMatrix();
								if (task->m_systemInfo->HasFlag(ParticleSystemInfo::Flag::FACE_TOWARDS_VIEWPORT)) {
									settings.viewportRight = Vector3(viewMatrix[0].x, viewMatrix[1].x, viewMatrix[2].x);
									settings.viewportUp = Vector3(viewMatrix[0].y, viewMatrix[1].y, viewMatrix[2].y);
									settings.flags |= FACE_TOWARDS_VIEWPORT;
								}

								// Check against frustrum:
								const RendererFrustrumDescriptor* viewport = subtaskPtr->viewport->ViewportFrustrumDescriptor();
								if (!Culling::FrustrumAABBCulling::Test(
									subtaskPtr->viewport->ProjectionMatrix() * viewMatrix,
									((viewport != nullptr) ? viewport : (const RendererFrustrumDescriptor*)subtaskPtr->viewport.operator->())->FrustrumTransform(),
									task->m_systemTransform, task->m_localSystemBoundaries, task->m_minOnScreenSize, task->m_maxOnScreenSize))
									continue;

								// If we have a shadowmapper, we should let the task know:
								if ((subtaskPtr->viewport->Flags() & RendererFrustrumFlags::SHADOWMAPPER) != RendererFrustrumFlags::NONE)
									settings.flags |= VIEWPORT_IS_A_SHADOWMAPPER;
							}

							// Update target buffer bindings:
							{
								settings.instanceBufferId = subtaskPtr->transformBuffer->Index();
								settings.indirectDrawBufferId = subtaskPtr->indirectDrawBuffer->Index();
								settings.indirectCommandIndex = static_cast<uint32_t>(subtaskPtr->indirectDrawCount->fetch_add(1u));
							}

							// Make sure indirect command index does not go out of scope:
							if (subtaskPtr->indirectDrawBuffer->BoundObject()->ObjectCount() <= settings.indirectCommandIndex) {
								m_context->Log()->Error(
									"ParticleInstanceBufferGenerator::Helpers::KernelInstance::Execute - Indirect draw index out of scope! ",
									"[File:", __FILE__, "; Line: ", __LINE__, "]");
								subtaskPtr->indirectDrawCount->fetch_sub(1u);
								continue;
							}

							// Update subtask settings and add it to the list:
							{
								subtaskPtr->task->SetSettings(settings);
								m_tasks.push_back(subtaskPtr->task);
								task->m_wasVisible = true;
							}
						}
					}
				}
				m_combinedKernel->Execute(commandBufferInfo, m_tasks.data(), m_tasks.size());
				m_tasks.clear();
			}
		};
	};

	ParticleInstanceBufferGenerator::ParticleInstanceBufferGenerator(ParticleSimulationStep* simulationStep)
		: GraphicsSimulation::Task(Helpers::Kernel::Instance(), simulationStep->Context())
		, m_simulationStep(simulationStep)
		, m_systemInfo(simulationStep->InitializationStep()->SystemInfo()) {}

	ParticleInstanceBufferGenerator::~ParticleInstanceBufferGenerator() {}

	void ParticleInstanceBufferGenerator::SetBuffers(ParticleBuffers* buffers) {
		std::unique_lock<SpinLock> lock(m_lock);
		m_newBuffers = buffers;
	}

	void ParticleInstanceBufferGenerator::Configure(size_t instanceBufferOffset, size_t objectIndex) {
		std::unique_lock<SpinLock> lock(m_lock);
		m_instanceStartIndex = instanceBufferOffset;
		m_objectIndex = objectIndex;
	}

	void ParticleInstanceBufferGenerator::BindViewportRange(const ViewportDescriptor* viewport,
		const Graphics::ArrayBufferReference<InstanceData> instanceBuffer,
		Graphics::IndirectDrawBuffer* indirectDrawBuffer,
		const std::shared_ptr<std::atomic<size_t>>& indirectDrawCount) {
		auto getBinding = [&](Graphics::ArrayBuffer* buffer, const auto&... errorMessage) -> Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> {
			if (buffer == nullptr) return nullptr;
			const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> binding = Context()->Graphics()->Bindless().Buffers()->GetBinding(buffer);
			if (binding == nullptr) Context()->Log()->Error("ParticleInstanceBufferGenerator::BindViewportRange - ", errorMessage...);
			return binding;
		};
		const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> instanceBufferId =
			getBinding(instanceBuffer, "Failed to bind instanceBuffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> indirectDrawBufferId =
			getBinding(indirectDrawBuffer, "Failed to bind indirectDrawBuffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		std::unique_lock<SpinLock> lock(m_lock);
		ViewportTask& task = [&]() -> ViewportTask& {
			// Get task if it's already there:
			{
				const auto it = m_viewportTasks.find(viewport);
				if (it != m_viewportTasks.end()) return m_viewTasks[it->second];
			}

			// Add new task:
			{
				ViewportTask task = {};
				if (viewport == nullptr)
					task.task = this;
				else {
					task.taskRef = Object::Instantiate<GraphicsSimulation::Task>(Helpers::Kernel::Instance(), viewport->Context());
					task.task = task.taskRef;
				}
				task.viewport = viewport;
				m_viewportTasks[viewport] = m_viewTasks.Size();
				m_viewTasks.Push(task);
			}

			return m_viewTasks[m_viewTasks.Size() - 1u];
		}();

		// Update ranges:
		{
			task.transformBuffer = instanceBufferId;
			task.indirectDrawBuffer = indirectDrawBufferId;
			task.indirectDrawCount = indirectDrawCount;
			if (task.indirectDrawCount != nullptr)
				task.indirectDrawCount->store(0u);
		}
	}

	void ParticleInstanceBufferGenerator::UnbindViewportRange(const ViewportDescriptor* viewport) {
		std::unique_lock<SpinLock> lock(m_lock);
		size_t index;

		// Find the index:
		{
			const auto it = m_viewportTasks.find(viewport);
			if (it == m_viewportTasks.end()) return;
			index = it->second;
			m_viewportTasks.erase(it);
		}

		// Swap index data with the last one:
		if (index != (m_viewTasks.Size() - 1u)) {
			ViewportTask& ptr = m_viewTasks[index];
			std::swap(ptr, m_viewTasks[m_viewTasks.Size() - 1u]);
			m_viewportTasks[ptr.viewport] = index;
		}

		// Remove last index:
		m_viewTasks.Pop();
	}

	void ParticleInstanceBufferGenerator::Synchronize() {
		// Update transform:
		m_systemTransform = m_systemInfo->WorldTransform();
		m_baseTransform = m_systemInfo->HasFlag(ParticleSystemInfo::Flag::SIMULATE_IN_LOCAL_SPACE) ? m_systemTransform : Math::Identity();
		m_systemInfo->GetCullingSettings(m_localSystemBoundaries, m_minOnScreenSize, m_maxOnScreenSize);
		m_independentParticleRotation = m_systemInfo->HasFlag(ParticleSystemInfo::Flag::INDEPENDENT_PARTICLE_ROTATION);
		m_simulateIfInvisible = !m_systemInfo->HasFlag(ParticleSystemInfo::Flag::DO_NOT_SIMULATE_IF_INVISIBLE);
		std::unique_lock<SpinLock> lock(m_lock);

		// Zero-out indirect draw counts:
		{
			const ViewportTask* subtaskPtr = m_viewTasks.Data();
			const ViewportTask* const subtaskEndPtr = subtaskPtr + m_viewTasks.Size();
			while (subtaskPtr < subtaskEndPtr) {
				if (subtaskPtr->indirectDrawCount != nullptr)
					subtaskPtr->indirectDrawCount->store(0u);
				subtaskPtr++;
			}
		}

		// If buffers have not changed, there's no need to go further:
		if (m_buffers == m_newBuffers) 
			return;
		m_buffers = m_newBuffers;

		// Get buffer indices:
		if (m_buffers != nullptr) {
			m_particleIndirectionBuffer = m_buffers->GetBuffer(ParticleBuffers::IndirectionBufferId());
			m_particleStateBuffer = m_buffers->GetBuffer(ParticleState::BufferId());
			m_liveParticleCountBuffer = m_buffers->LiveParticleCountBuffer();
			if (m_particleIndirectionBuffer == nullptr || m_particleStateBuffer == nullptr || m_liveParticleCountBuffer == nullptr) {
				Context()->Log()->Error("ParticleInstanceBufferGenerator::Synchronize - Failed to get buffer bindings! [File:", __FILE__, "; Line: ", __LINE__, "]");
				m_buffers = nullptr;
			}
		}

		// If we do not have ParticleBuffers reference, cleanup is due:
		if (m_buffers == nullptr) {
			m_particleIndirectionBuffer = nullptr;
			m_particleStateBuffer = nullptr;
			m_liveParticleCountBuffer = nullptr;
		}
	}

	void ParticleInstanceBufferGenerator::GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const {
		if (m_simulateIfInvisible || m_wasVisible)
			recordDependency(m_simulationStep);
	}
}

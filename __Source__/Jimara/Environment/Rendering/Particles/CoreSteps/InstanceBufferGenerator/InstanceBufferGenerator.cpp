#include "InstanceBufferGenerator.h"
#include "../../ParticleState.h"
#include "../../../../GraphicsSimulation/CombinedGraphicsSimulationKernel.h"


namespace Jimara {
	struct ParticleInstanceBufferGenerator::Helpers {
		/// <summary>
		/// Settings for a task
		/// </summary>
		struct JIMARA_API TaskSettings {
			/// <summary> 
			/// World matrix of the particle system, if the simulation runs in local space, Math::Identity() otherwise;
			/// Same, but multiplied by a viewport-facing rotation if we have camera-facing quads instead of meshes.
			/// </summary>
			alignas(16) Matrix4 baseTransform = Math::Identity();	// Bytes [0 - 64)

			/// <summary> Indirection/Index Wrangle bindless buffer id </summary>
			alignas(4) uint32_t particleIndirectionBufferId = 0u;	// Bytes [64 - 68)

			/// <summary> Bindless buffer id for ParticleState </summary>
			alignas(4) uint32_t particleStateBufferId = 0u;			// Bytes [68 - 72)

			/// <summary> Bindless buffer id for the resulting Matrix4 instance buffer </summary>
			alignas(4) uint32_t instanceBufferId = 0u;				// Bytes [72 - 76)

			/// <summary> Index of the first particle's instance within the instanceBuffer </summary>
			alignas(4) uint32_t instanceStartId = 0u;				// Bytes [76 - 80)

			/// <summary> Number of particles within the particle system (name is important; ) </summary>
			alignas(4) uint32_t taskThreadCount = 0u;				// Bytes [80 - 84)

			/// <summary> Bindless buffer id for the 'Live Particle Count' buffer </summary>
			alignas(4) uint32_t liveParticleCountBufferId = 0u;		// Bytes [84 - 88)

			/// <summary> Bindless buffer id for the 'Indirect draw buffer' </summary>
			alignas(4) uint32_t indirectDrawBufferId = 0u;			// Bytes [88 - 92)

			/// <summary> Index of the particle system within the Indirect draw buffer </summary>
			alignas(4) uint32_t indirectCommandIndex = 0u;			// Bytes [92 - 96)
		};
		static_assert(sizeof(TaskSettings) == 96);

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
				static const Graphics::ShaderClass SHADER_CLASS("Jimara/Environment/Rendering/Particles/CoreSteps/InstanceBufferGenerator/InstanceBufferGenerator_Kernel");
				struct BindingSet : public virtual Graphics::ShaderResourceBindings::ShaderResourceBindingSet {
					const Reference<Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> binding =
						Object::Instantiate<Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding>();
					inline virtual Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> FindConstantBufferBinding(const std::string&)const override { return nullptr; }
					inline virtual Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> FindStructuredBufferBinding(const std::string&)const override { return nullptr; }
					inline virtual Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> FindTextureSamplerBinding(const std::string&)const override { return nullptr; }
					inline virtual Reference<const Graphics::ShaderResourceBindings::TextureViewBinding> FindTextureViewBinding(const std::string&)const override { return nullptr; }
					inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> FindBindlessStructuredBufferSetBinding(const std::string&)const override { return binding; }
					inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureSamplerSetBinding> FindBindlessTextureSamplerSetBinding(const std::string&)const override { return nullptr; }
					inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureViewSetBinding> FindBindlessTextureViewSetBinding(const std::string&)const override { return nullptr; }
				} bindingSet;
				bindingSet.binding->BoundObject() = context->Graphics()->Bindless().BufferBinding();
				Reference<GraphicsSimulation::KernelInstance> kernel = CombinedGraphicsSimulationKernel<TaskSettings>::Create(context, &SHADER_CLASS, bindingSet);
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

			virtual void Execute(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, const Task* const* tasks, size_t taskCount) final override {
				m_tasks.clear();
				const Task* const* ptr = tasks;
				const Task* const* const end = ptr + taskCount;
				while (ptr < end) {
					const ParticleInstanceBufferGenerator* task = dynamic_cast<const ParticleInstanceBufferGenerator*>(*ptr);
					if (task != nullptr) {
						std::unique_lock<SpinLock> lock(task->m_lock);
						if (task->m_buffers == nullptr) continue;

						const Matrix4 baseTransform = task->m_baseTransform;
						std::optional<Vector3> baseScale;

						// Extract common settings:
						TaskSettings settings = {};
						{
							settings.particleIndirectionBufferId = task->m_particleIndirectionBuffer->Index();
							settings.particleStateBufferId = task->m_particleStateBuffer->Index();
							settings.liveParticleCountBufferId = task->m_liveParticleCountBuffer->Index();
							settings.taskThreadCount = static_cast<uint32_t>(task->m_buffers->ParticleBudget());
						}

						const ViewportTask* subtaskPtr = task->m_viewTasks.Data();
						const ViewportTask* const subtaskEndPtr = subtaskPtr + task->m_viewTasks.Size();
						while (subtaskPtr < subtaskEndPtr) {
							// Discard subtask if target buffer bindings are missing:
							if (subtaskPtr->transformBuffer == nullptr || subtaskPtr->indirectDrawBuffer == nullptr) {
								subtaskPtr++;
								continue;
							}

							// Update target buffer bindings:
							{
								settings.instanceBufferId = subtaskPtr->transformBuffer->Index();
								settings.instanceStartId = static_cast<uint32_t>(subtaskPtr->transformStartIndex);
								settings.indirectDrawBufferId = subtaskPtr->indirectDrawBuffer->Index();
								settings.indirectCommandIndex = static_cast<uint32_t>(subtaskPtr->indirectDrawIndex);
							}

							// Update transform:
							if (subtaskPtr->viewport == nullptr)
								settings.baseTransform = baseTransform;
							else {
								const Matrix4 viewportTransform = Math::Inverse(subtaskPtr->viewport->ViewMatrix());
								if (!baseScale.has_value())
									baseScale = Vector3(Math::Magnitude(baseTransform[0]), Math::Magnitude(baseTransform[1]), Math::Magnitude(baseTransform[2]));
								const Vector3& scale = baseScale.value();
								settings.baseTransform[0] = viewportTransform[0] * scale.x;
								settings.baseTransform[1] = viewportTransform[1] * scale.y;
								settings.baseTransform[2] = viewportTransform[2] * scale.z;
								settings.baseTransform[3] = baseTransform[3];
							}

							// Update subtask settings and add it to the list:
							{
								subtaskPtr->task->SetSettings(settings);
								m_tasks.push_back(subtaskPtr->task);
								subtaskPtr++;
							}
						}
					}
					else m_context->Log()->Warning("ParticleInstanceBufferGenerator::Helpers::KernelInstance::Execute - Got unsupported task! [File:", __FILE__, "; Line: ", __LINE__, "]");
					ptr++;
				}
				m_combinedKernel->Execute(commandBufferInfo, m_tasks.data(), m_tasks.size());
				m_tasks.clear();
			}
		};
	};

	ParticleInstanceBufferGenerator::ParticleInstanceBufferGenerator(GraphicsSimulation::Task* simulationStep)
		: GraphicsSimulation::Task(Helpers::Kernel::Instance(), simulationStep->Context())
		, m_simulationStep(simulationStep) {}

	ParticleInstanceBufferGenerator::~ParticleInstanceBufferGenerator() {}

	void ParticleInstanceBufferGenerator::SetBuffers(ParticleBuffers* buffers) {
		std::unique_lock<SpinLock> lock(m_lock);
		if (m_buffers == buffers) return;
		m_buffers = buffers;

		// Get buffer indices:
		if (m_buffers != nullptr) {
			m_particleIndirectionBuffer = m_buffers->GetBuffer(ParticleBuffers::IndirectionBufferId());
			m_particleStateBuffer = m_buffers->GetBuffer(ParticleState::BufferId());
			m_liveParticleCountBuffer = m_buffers->LiveParticleCountBuffer();
			if (m_particleIndirectionBuffer == nullptr || m_particleStateBuffer == nullptr || m_liveParticleCountBuffer == nullptr) {
				Context()->Log()->Error("ParticleInstanceBufferGenerator::SetBuffers - Failed to get buffer bindings! [File:", __FILE__, "; Line: ", __LINE__, "]");
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

	void ParticleInstanceBufferGenerator::SetTransform(const Matrix4& transform) {
		std::unique_lock<SpinLock> lock(m_lock);
		m_baseTransform = transform;
	}

	void ParticleInstanceBufferGenerator::BindViewportRanges(const ViewportDescriptor* viewport,
		const Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* instanceBuffer, size_t startIndex,
		const Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* indirectDrawBuffer, size_t indirectIndex) {
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
			task.transformBuffer = instanceBuffer;
			task.transformStartIndex = startIndex;
			task.indirectDrawBuffer = indirectDrawBuffer;
			task.indirectDrawIndex = indirectIndex;
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

	void ParticleInstanceBufferGenerator::GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const {
		recordDependency(m_simulationStep);
	}
}

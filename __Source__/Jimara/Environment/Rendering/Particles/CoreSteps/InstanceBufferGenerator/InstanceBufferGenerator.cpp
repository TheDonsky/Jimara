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

						// Extract common settings:
						TaskSettings settings = {};
						{
							settings.particleIndirectionBufferId = task->m_particleIndirectionBuffer->Index();
							settings.particleStateBufferId = task->m_particleStateBuffer->Index();
							settings.liveParticleCountBufferId = task->m_liveParticleCountBuffer->Index();
							settings.taskThreadCount = static_cast<uint32_t>(task->m_buffers->ParticleBudget());
							settings.instanceStartId = static_cast<uint32_t>(task->m_instanceStartIndex);
							settings.indirectCommandIndex = static_cast<uint32_t>(task->m_indirectDrawIndex);
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
								settings.indirectDrawBufferId = subtaskPtr->indirectDrawBuffer->Index();
							}

							// Update transform:
							settings.baseTransform = baseTransform;
							if (subtaskPtr->viewport != nullptr) {
								const Matrix4 viewMatrix = subtaskPtr->viewport->ViewMatrix();
								settings.viewportRight = Vector3(viewMatrix[0].x, viewMatrix[1].x, viewMatrix[2].x);
								settings.viewportUp = Vector3(viewMatrix[0].y, viewMatrix[1].y, viewMatrix[2].y);
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
		m_newBuffers = buffers;
	}

	void ParticleInstanceBufferGenerator::Configure(const Matrix4& transform, size_t instanceBufferOffset, size_t indirectDrawIndex) {
		std::unique_lock<SpinLock> lock(m_lock);
		m_baseTransform = transform;
		m_instanceStartIndex = instanceBufferOffset;
		m_indirectDrawIndex = indirectDrawIndex;
	}

	void ParticleInstanceBufferGenerator::BindViewportRange(const ViewportDescriptor* viewport,
		const Graphics::ArrayBufferReference<InstanceData> instanceBuffer,
		Graphics::IndirectDrawBuffer* indirectDrawBuffer) {
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
		std::unique_lock<SpinLock> lock(m_lock);
		if (m_buffers == m_newBuffers) return;
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
		recordDependency(m_simulationStep);
	}
}

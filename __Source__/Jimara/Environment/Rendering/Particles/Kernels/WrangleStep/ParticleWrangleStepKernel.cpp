#include "ParticleWrangleStepKernel.h"
#include "../../ParticleState.h"
#include "../../../Algorithms/SegmentTree/SegmentTreeGenerationKernel.h"


namespace Jimara {
	struct ParticleWrangleStepKernel::Helpers {
		struct ParticleTaskSettings {
			alignas(4) uint32_t particleStateBufferId = 0u;			// Bytes [0 - 4)
			alignas(4) uint32_t particleIndirectionBufferId = 0u;	// Bytes [4 - 8)
			alignas(4) uint32_t liveParticleCountBufferId = 0u;		// Bytes [12 - 16)
			alignas(4) uint32_t particleCount = 0u;					// Bytes [8 - 12)
		};

		static const ParticleWrangleStepKernel* Instance() {
			static const ParticleWrangleStepKernel instance;
			return &instance;
		}

		class KernelInstance : public virtual ParticleKernel::Instance {
		private:
			const Reference<SceneContext> m_context;
			const Reference<ParticleKernel::Instance> m_liveCheckKernel;
			const Reference<SegmentTreeGenerationKernel> m_segmentTreeGenerator;
			const Reference<ParticleKernel::Instance> m_indirectionUpdateKernel;
			const Reference<Graphics::ShaderResourceBindings::StructuredBufferBinding> m_segmentTreeBinding;

		public:
			inline virtual void Execute(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, const ParticleKernel::Task* const* tasks, size_t taskCount)override {
				// Count total number of particles:
				const size_t particleCount = [&]() {
					size_t count = 0u;
					const ParticleKernel::Task* const* taskPtr = tasks;
					const ParticleKernel::Task* const* const end = taskPtr + taskCount;
					while (taskPtr < end) {
						count += (*taskPtr)->GetSettings<ParticleTaskSettings>().particleCount;
						taskPtr++;
					}
					return count;
				}();

				// (Re)Allocate segment tree if needed:
				const size_t segmentTreeSize = SegmentTreeGenerationKernel::SegmentTreeBufferSize(particleCount);
				if (m_segmentTreeBinding->BoundObject() == nullptr || m_segmentTreeBinding->BoundObject()->ObjectCount() < segmentTreeSize) {
					m_segmentTreeBinding->BoundObject() = m_context->Graphics()->Device()->CreateArrayBuffer<uint32_t>(
						Math::Max((m_segmentTreeBinding->BoundObject() == nullptr) ? size_t(0u) : m_segmentTreeBinding->BoundObject()->ObjectCount() << 1u, segmentTreeSize));
					if (m_segmentTreeBinding->BoundObject() == nullptr) {
						m_context->Log()->Error("ParticleWrangleStepKernel::KernelInstance::Execute - Failed to allocate buffer for the segment tree! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						return;
					}
				}

				// Execute pipelines:
				m_liveCheckKernel->Execute(commandBufferInfo, tasks, taskCount);
				m_segmentTreeGenerator->Execute(commandBufferInfo, m_segmentTreeBinding->BoundObject(), particleCount, true);
				m_indirectionUpdateKernel->Execute(commandBufferInfo, tasks, taskCount);
			}
		};
	};

	ParticleWrangleStepKernel::Task::Task(SceneContext* context)
		: ParticleKernel::Task(Helpers::Instance(), context) {
		Helpers::ParticleTaskSettings settings = {};
		SetSettings(settings);
	}

	ParticleWrangleStepKernel::Task::~Task() {}

	void ParticleWrangleStepKernel::Task::SetBuffers(ParticleBuffers* buffers) {
		std::unique_lock<SpinLock> lock(m_bufferLock);
		m_buffers = buffers;
	}

	void ParticleWrangleStepKernel::Task::Synchronize() {
		const Reference<ParticleBuffers> buffers = [&]() ->Reference<ParticleBuffers> {
			std::unique_lock<SpinLock> lock(m_bufferLock);
			Reference<ParticleBuffers> rv = m_buffers;
			return rv;
		}();
		if (m_lastBuffers == buffers) return;

		Helpers::ParticleTaskSettings settings = {};
		if (buffers != nullptr) {
			const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> particleStateBuffer = buffers->GetBuffer(ParticleState::BufferId());
			if (particleStateBuffer == nullptr)
				Context()->Log()->Error("ParticleWrangleStepKernel::Task::Synchronize - Failed to get ParticleState buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			else {
				const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> indirectionBuffer = buffers->GetBuffer(ParticleBuffers::IndirectionBufferId());
				if (indirectionBuffer == nullptr)
					Context()->Log()->Error("ParticleWrangleStepKernel::Task::Synchronize - Failed to get indirection buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				else if (buffers->ParticleBudget() > 0u) {
					settings.particleStateBufferId = particleStateBuffer->Index();
					settings.particleIndirectionBufferId = indirectionBuffer->Index();
					settings.liveParticleCountBufferId = buffers->LiveParticleCountBuffer()->Index();
					settings.particleCount = static_cast<uint32_t>(buffers->ParticleBudget());
				}
			}
		}

		m_lastBuffers = (settings.particleCount > 0u) ? buffers : nullptr;
		SetSettings(settings);
	}

	ParticleWrangleStepKernel::ParticleWrangleStepKernel() : ParticleKernel(sizeof(Helpers::ParticleTaskSettings)) {}
	ParticleWrangleStepKernel::~ParticleWrangleStepKernel() {}

	Reference<ParticleKernel::Instance> ParticleWrangleStepKernel::CreateInstance(SceneContext* context)const {
		if (context == nullptr) return nullptr;
		context->Log()->Error("ParticleWrangleStepKernel::CreateInstance - Not yet implemented!");
		return nullptr;
	}
}

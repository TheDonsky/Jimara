#include "ParticleWrangleStepKernel.h"
#include "../../ParticleState.h"
#include "../../../Algorithms/SegmentTree/SegmentTreeGenerationKernel.h"
#include "../../../../GraphicsSimulation/CombinedGraphicsSimulationKernel.h"


namespace Jimara {
	struct ParticleWrangleStepKernel::Helpers {
		struct ParticleTaskSettings {
			alignas(4) uint32_t particleStateBufferId = 0u;			// Bytes [0 - 4)
			alignas(4) uint32_t particleIndirectionBufferId = 0u;	// Bytes [4 - 8)
			alignas(4) uint32_t liveParticleCountBufferId = 0u;		// Bytes [12 - 16)
			alignas(4) uint32_t taskThreadCount = 0u;				// Bytes [8 - 12)
		};

		static const ParticleWrangleStepKernel* Instance() {
			static const ParticleWrangleStepKernel instance;
			return &instance;
		}

		class KernelInstance : public virtual GraphicsSimulation::KernelInstance {
		private:
			const Reference<SceneContext> m_context;
			const Reference<GraphicsSimulation::KernelInstance> m_liveCheckKernel;
			const Reference<SegmentTreeGenerationKernel> m_segmentTreeGenerator;
			const Reference<GraphicsSimulation::KernelInstance> m_indirectionUpdateKernel;
			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_segmentTreeBinding;
			const Graphics::BufferReference<uint32_t> m_totalParticleCountBuffer;

		public:
			inline KernelInstance(
				SceneContext* context, 
				GraphicsSimulation::KernelInstance* liveCheckKernel,
				SegmentTreeGenerationKernel* segmentTreeGenerator,
				GraphicsSimulation::KernelInstance* indirectionUpdateKernel,
				Graphics::ResourceBinding<Graphics::ArrayBuffer>* segmentTreeBinding,
				Graphics::Buffer* totalParticleCountBuffer) 
				: m_context(context)
				, m_liveCheckKernel(liveCheckKernel)
				, m_segmentTreeGenerator(segmentTreeGenerator)
				, m_indirectionUpdateKernel(indirectionUpdateKernel)
				, m_segmentTreeBinding(segmentTreeBinding)
				, m_totalParticleCountBuffer(totalParticleCountBuffer) {}

			inline virtual ~KernelInstance() {}

			inline virtual void Execute(Graphics::InFlightBufferInfo commandBufferInfo, const GraphicsSimulation::Task* const* tasks, size_t taskCount)override {
				// Count total number of particles:
				const size_t particleCount = [&]() {
					size_t count = 0u;
					const GraphicsSimulation::Task* const* taskPtr = tasks;
					const GraphicsSimulation::Task* const* const end = taskPtr + taskCount;
					while (taskPtr < end) {
						count += (*taskPtr)->GetSettings<ParticleTaskSettings>().taskThreadCount;
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

				// Update total particle count:
				{
					m_totalParticleCountBuffer.Map() = static_cast<uint32_t>(particleCount);
					m_totalParticleCountBuffer->Unmap(true);
				}

				// Execute pipelines:
				{
					m_liveCheckKernel->Execute(commandBufferInfo, tasks, taskCount);
					m_segmentTreeGenerator->Execute(commandBufferInfo, m_segmentTreeBinding->BoundObject(), particleCount, true);
					m_indirectionUpdateKernel->Execute(commandBufferInfo, tasks, taskCount);
				}
			}
		};
	};

	ParticleWrangleStepKernel::Task::Task(SceneContext* context,
		Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* particleState,
		Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* indirectionBuffer,
		Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* liveParticleCount)
		: GraphicsSimulation::Task(Helpers::Instance(), context)
		, m_particleState(particleState)
		, m_indirectionBuffer(indirectionBuffer)
		, m_liveParticleCount(liveParticleCount) {
		Helpers::ParticleTaskSettings settings = {};
		
		if (m_particleState == nullptr)
			Context()->Log()->Error("ParticleWrangleStepKernel::Task::Task - particleState not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		else settings.particleStateBufferId = m_particleState->Index();
		
		if (m_indirectionBuffer == nullptr)
			Context()->Log()->Error("ParticleWrangleStepKernel::Task::Task - indirectionBuffer not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		else settings.particleIndirectionBufferId = m_indirectionBuffer->Index();
		
		if (m_liveParticleCount == nullptr)
			Context()->Log()->Error("ParticleWrangleStepKernel::Task::Task - liveParticleCount not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		else settings.liveParticleCountBufferId = m_liveParticleCount->Index();
		
		if (m_particleState != nullptr && m_indirectionBuffer != nullptr && m_liveParticleCount != nullptr) {
			if (m_particleState->BoundObject()->ObjectCount() != m_indirectionBuffer->BoundObject()->ObjectCount())
				Context()->Log()->Error(
					"ParticleWrangleStepKernel::Task::Task - particleState and indirectionBuffer element count mismatch! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			else settings.taskThreadCount = static_cast<uint32_t>(m_particleState->BoundObject()->ObjectCount());
		}
		
		SetSettings(settings);
	}

	ParticleWrangleStepKernel::Task::~Task() {}

	ParticleWrangleStepKernel::ParticleWrangleStepKernel() : GraphicsSimulation::Kernel(sizeof(Helpers::ParticleTaskSettings)) {}
	ParticleWrangleStepKernel::~ParticleWrangleStepKernel() {}

	Reference<GraphicsSimulation::KernelInstance> ParticleWrangleStepKernel::CreateInstance(SceneContext* context)const {
		if (context == nullptr) return nullptr;
		auto error = [&](auto... message) {
			context->Log()->Error("ParticleWrangleStepKernel::CreateInstance - ", message...);
			return nullptr;
		};

		const Reference<Graphics::ResourceBinding<Graphics::Buffer>> totalParticleCountBinding =
			Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>();
		totalParticleCountBinding->BoundObject() = context->Graphics()->Device()->CreateConstantBuffer<uint32_t>();
		if (totalParticleCountBinding->BoundObject() == nullptr)
			return error("Failed to create settings buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		auto findTotalParticleCount = [&](const auto& info) { 
			static const constexpr std::string_view TOTAL_PARTICLE_COUNT_BINDING_NAME = "totalParticleCount";
			return (info.name == TOTAL_PARTICLE_COUNT_BINDING_NAME) ? totalParticleCountBinding : nullptr;
		};

		const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> segmentTreeBufferBinding =
			Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
		auto findSegmentTreeBufferBinding = [&](const auto& info) { 
			static const constexpr std::string_view SEGMENT_TREE_BUFFER_BINDING_NAME = "segmentTreeBuffer";
			return (info.name == SEGMENT_TREE_BUFFER_BINDING_NAME) ? segmentTreeBufferBinding : nullptr;
		};

		Graphics::BindingSet::BindingSearchFunctions bindings = {};
		bindings.constantBuffer = &findTotalParticleCount;
		bindings.structuredBuffer = &findSegmentTreeBufferBinding;

		static const Graphics::ShaderClass liveCheckKernelShaderClass(
			"Jimara/Environment/Rendering/Particles/CoreSteps/WrangleStep/ParticleWrangleStep_LiveCheckKernel");
		const Reference<GraphicsSimulation::KernelInstance> liveCheckKernel = CombinedGraphicsSimulationKernel<Helpers::ParticleTaskSettings>::Create(
			context, &liveCheckKernelShaderClass, bindings);
		if (liveCheckKernel == nullptr)
			return error("Failed to create 'Live Check' kernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<SegmentTreeGenerationKernel> segmentTreeGenerator = SegmentTreeGenerationKernel::CreateUintSumKernel(
			context->Graphics()->Device(), context->Graphics()->Configuration().ShaderLibrary(), context->Graphics()->Configuration().MaxInFlightCommandBufferCount());
		if (segmentTreeGenerator == nullptr) 
			return error("Failed to create segment tree generator! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		static const Graphics::ShaderClass indirectionUpdateKernelShaderClass(
			"Jimara/Environment/Rendering/Particles/CoreSteps/WrangleStep/ParticleWrangleStep_IndirectUpdateKernel");
		const Reference<GraphicsSimulation::KernelInstance> indirectionUpdateKernel = CombinedGraphicsSimulationKernel<Helpers::ParticleTaskSettings>::Create(
			context, &indirectionUpdateKernelShaderClass, bindings);
		if (indirectionUpdateKernel == nullptr)
			return error("Failed to create 'Indirect Update' kernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		return Object::Instantiate<Helpers::KernelInstance>(
			context, liveCheckKernel, segmentTreeGenerator, indirectionUpdateKernel,
			segmentTreeBufferBinding, totalParticleCountBinding->BoundObject());
	}
}

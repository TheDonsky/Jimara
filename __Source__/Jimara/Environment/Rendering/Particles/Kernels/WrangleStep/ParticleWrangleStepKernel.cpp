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
			const Reference<Graphics::ShaderResourceBindings::StructuredBufferBinding> m_segmentTreeBinding;
			const Graphics::BufferReference<uint32_t> m_totalParticleCountBuffer;

		public:
			inline KernelInstance(
				SceneContext* context, 
				GraphicsSimulation::KernelInstance* liveCheckKernel,
				SegmentTreeGenerationKernel* segmentTreeGenerator,
				GraphicsSimulation::KernelInstance* indirectionUpdateKernel,
				Graphics::ShaderResourceBindings::StructuredBufferBinding* segmentTreeBinding,
				Graphics::Buffer* totalParticleCountBuffer) 
				: m_context(context)
				, m_liveCheckKernel(liveCheckKernel)
				, m_segmentTreeGenerator(segmentTreeGenerator)
				, m_indirectionUpdateKernel(indirectionUpdateKernel)
				, m_segmentTreeBinding(segmentTreeBinding)
				, m_totalParticleCountBuffer(totalParticleCountBuffer) {}

			inline virtual ~KernelInstance() {}

			inline virtual void Execute(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, const GraphicsSimulation::Task* const* tasks, size_t taskCount)override {
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

		struct BindingSet : public virtual Graphics::ShaderResourceBindings::ShaderResourceBindingSet {
			const Reference<Graphics::ShaderResourceBindings::ConstantBufferBinding> totalParticleCountBinding =
				Object::Instantiate<Graphics::ShaderResourceBindings::ConstantBufferBinding>();
			const Reference<Graphics::ShaderResourceBindings::StructuredBufferBinding> segmentTreeBufferBinding =
				Object::Instantiate<Graphics::ShaderResourceBindings::StructuredBufferBinding>();
			const Reference<Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> bindlessBinding =
				Object::Instantiate<Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding>();

			inline virtual Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> FindConstantBufferBinding(const std::string& name)const override { 
				static const std::string TOTAL_PARTICLE_COUNT_BINDING_NAME = "totalParticleCount";
				return (name == TOTAL_PARTICLE_COUNT_BINDING_NAME) ? totalParticleCountBinding : nullptr;
			}
			inline virtual Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> FindStructuredBufferBinding(const std::string& name)const override { 
				static const std::string SEGMENT_TREE_BUFFER_BINDING_NAME = "segmentTreeBuffer";
				return (name == SEGMENT_TREE_BUFFER_BINDING_NAME) ? segmentTreeBufferBinding : nullptr;
			}
			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> FindTextureSamplerBinding(const std::string&)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureViewBinding> FindTextureViewBinding(const std::string&)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> FindBindlessStructuredBufferSetBinding(const std::string&)const override { return bindlessBinding; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureSamplerSetBinding> FindBindlessTextureSamplerSetBinding(const std::string&)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureViewSetBinding> FindBindlessTextureViewSetBinding(const std::string&)const override { return nullptr; }
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

		Helpers::BindingSet bindingSet = {};
		{
			bindingSet.totalParticleCountBinding->BoundObject() = context->Graphics()->Device()->CreateConstantBuffer<uint32_t>();
			if (bindingSet.totalParticleCountBinding->BoundObject() == nullptr) 
				return error("Failed to create settings buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			bindingSet.bindlessBinding->BoundObject() = context->Graphics()->Bindless().BufferBinding();
		}

		static const Graphics::ShaderClass liveCheckKernelShaderClass("Jimara/Environment/Rendering/Particles/Kernels/WrangleStep/ParticleWrangleStep_LiveCheckKernel");
		const Reference<GraphicsSimulation::KernelInstance> liveCheckKernel = CombinedGraphicsSimulationKernel<Helpers::ParticleTaskSettings>::Create(context, &liveCheckKernelShaderClass, bindingSet);
		if (liveCheckKernel == nullptr)
			return error("Failed to create 'Live Check' kernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<SegmentTreeGenerationKernel> segmentTreeGenerator = SegmentTreeGenerationKernel::CreateUintSumKernel(
			context->Graphics()->Device(), context->Graphics()->Configuration().ShaderLoader(), context->Graphics()->Configuration().MaxInFlightCommandBufferCount());
		if (segmentTreeGenerator == nullptr) 
			return error("Failed to create segment tree generator! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		static const Graphics::ShaderClass indirectionUpdateKernelShaderClass("Jimara/Environment/Rendering/Particles/Kernels/WrangleStep/ParticleWrangleStep_IndirectUpdateKernel");
		const Reference<GraphicsSimulation::KernelInstance> indirectionUpdateKernel = CombinedGraphicsSimulationKernel<Helpers::ParticleTaskSettings>::Create(context, &indirectionUpdateKernelShaderClass, bindingSet);
		if (indirectionUpdateKernel == nullptr)
			return error("Failed to create 'Indirect Update' kernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		return Object::Instantiate<Helpers::KernelInstance>(
			context, liveCheckKernel, segmentTreeGenerator, indirectionUpdateKernel,
			bindingSet.segmentTreeBufferBinding, bindingSet.totalParticleCountBinding->BoundObject());
	}
}

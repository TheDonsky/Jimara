#include "ParticleWrangleStepKernel.h"
#include "../CombinedParticleKernel.h"
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
			const Graphics::BufferReference<uint32_t> m_totalParticleCountBuffer;

		public:
			inline KernelInstance(
				SceneContext* context, 
				ParticleKernel::Instance* liveCheckKernel, 
				SegmentTreeGenerationKernel* segmentTreeGenerator,
				ParticleKernel::Instance* indirectionUpdateKernel,
				Graphics::ShaderResourceBindings::StructuredBufferBinding* segmentTreeBinding,
				Graphics::Buffer* totalParticleCountBuffer) 
				: m_context(context)
				, m_liveCheckKernel(liveCheckKernel)
				, m_segmentTreeGenerator(segmentTreeGenerator)
				, m_indirectionUpdateKernel(indirectionUpdateKernel)
				, m_segmentTreeBinding(segmentTreeBinding)
				, m_totalParticleCountBuffer(totalParticleCountBuffer) {}

			inline virtual ~KernelInstance() {}

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

			inline virtual Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> FindConstantBufferBinding(const std::string&)const override { return totalParticleCountBinding; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> FindStructuredBufferBinding(const std::string&)const override { return segmentTreeBufferBinding; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> FindTextureSamplerBinding(const std::string&)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureViewBinding> FindTextureViewBinding(const std::string&)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> FindBindlessStructuredBufferSetBinding(const std::string&)const override { return bindlessBinding; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureSamplerSetBinding> FindBindlessTextureSamplerSetBinding(const std::string&)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureViewSetBinding> FindBindlessTextureViewSetBinding(const std::string&)const override { return nullptr; }
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
		const Reference<ParticleKernel::Instance> liveCheckKernel = CombinedParticleKernel<Helpers::ParticleTaskSettings>::Create(context, &liveCheckKernelShaderClass, bindingSet);
		if (liveCheckKernel == nullptr)
			return error("Failed to create 'Live Check' kernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<SegmentTreeGenerationKernel> segmentTreeGenerator = SegmentTreeGenerationKernel::CreateUintSumKernel(
			context->Graphics()->Device(), context->Graphics()->Configuration().ShaderLoader(), context->Graphics()->Configuration().MaxInFlightCommandBufferCount());
		if (segmentTreeGenerator == nullptr) 
			return error("Failed to create segment tree generator! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		static const Graphics::ShaderClass indirectionUpdateKernelShaderClass("Jimara/Environment/Rendering/Particles/Kernels/WrangleStep/ParticleWrangleStep_IndirectUpdateKernel");
		const Reference<ParticleKernel::Instance> indirectionUpdateKernel = CombinedParticleKernel<Helpers::ParticleTaskSettings>::Create(context, &indirectionUpdateKernelShaderClass, bindingSet);
		if (indirectionUpdateKernel == nullptr)
			return error("Failed to create 'Indirect Update' kernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		return Object::Instantiate<Helpers::KernelInstance>(
			context, liveCheckKernel, segmentTreeGenerator, indirectionUpdateKernel,
			bindingSet.segmentTreeBufferBinding, bindingSet.totalParticleCountBinding->BoundObject());
	}
}

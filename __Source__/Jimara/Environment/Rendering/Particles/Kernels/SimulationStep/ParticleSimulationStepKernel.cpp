#include "ParticleSimulationStepKernel.h"
#include "../CombinedParticleKernel.h"
#include "../../ParticleState.h"

namespace Jimara {
	struct ParticleSimulationStepKernel::Helpers {
		struct ParticleTaskSettings {
			alignas(4) uint32_t particleStateBufferId = 0u;		// Bytes [0 - 4)
			alignas(4) uint32_t particleCount = 0u;				// Bytes [4 - 8)
			alignas(4) float timeScale = 0.0f;					// Bytes [8 - 12)
			alignas(4) uint32_t timeType = 0u;					// Bytes [12 - 16) (0 - scaled; 1 - unscaled)
		};

		static const ParticleSimulationStepKernel* Instance() {
			static const ParticleSimulationStepKernel instance;
			return &instance;
		}

		class KernelInstance : public virtual ParticleKernel::Instance {
		private:
			const Reference<SceneContext> m_context;
			const Graphics::BufferReference<Vector4> m_timeInfo;
			const Reference<ParticleKernel::Instance> m_kernel;
			

		public:
			inline KernelInstance(SceneContext* context, Graphics::BufferReference<Vector4>& timeInfo, ParticleKernel::Instance* kernel)
				: m_context(context), m_timeInfo(timeInfo), m_kernel(kernel) {}

			inline virtual ~KernelInstance() {}

			inline virtual void Execute(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, const ParticleKernel::Task* const* tasks, size_t taskCount) override {
				const Vector4 timeInfo = Vector4(
					0.0f,
					m_context->Time()->UnscaledDeltaTime(),
					m_context->Time()->ScaledDeltaTime(),
					m_context->Physics()->Time()->ScaledDeltaTime());
				m_timeInfo.Map() = timeInfo;
				m_timeInfo->Unmap(true);
				m_kernel->Execute(commandBufferInfo, tasks, taskCount);
			}
		};

		struct BindingSet : public virtual Graphics::ShaderResourceBindings::ShaderResourceBindingSet {
			const Reference<Graphics::ShaderResourceBindings::ConstantBufferBinding> constantBinding =
				Object::Instantiate<Graphics::ShaderResourceBindings::ConstantBufferBinding>();
			const Reference<Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> bindlessBinding =
				Object::Instantiate<Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding>();
			
			inline BindingSet(SceneContext* context, Graphics::Buffer* constantBuffer) {
				constantBinding->BoundObject() = constantBuffer;
				bindlessBinding->BoundObject() = context->Graphics()->Bindless().BufferBinding(); 
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> FindConstantBufferBinding(const std::string&)const override { return constantBinding; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> FindStructuredBufferBinding(const std::string&)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> FindTextureSamplerBinding(const std::string&)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureViewBinding> FindTextureViewBinding(const std::string&)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> FindBindlessStructuredBufferSetBinding(const std::string&)const override { return bindlessBinding; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureSamplerSetBinding> FindBindlessTextureSamplerSetBinding(const std::string&)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureViewSetBinding> FindBindlessTextureViewSetBinding(const std::string&)const override { return nullptr; }
		};
	};


	ParticleSimulationStepKernel::Task::Task(SceneContext* context) 
		: ParticleKernel::Task(Helpers::Instance(), context) {}

	ParticleSimulationStepKernel::Task::~Task() {}

	void ParticleSimulationStepKernel::Task::SetBuffers(ParticleBuffers* buffers) {
		std::unique_lock<SpinLock> lock(m_bufferLock);
		m_buffers = buffers;
	}

	void ParticleSimulationStepKernel::Task::SetTimeScale(float timeScale) {
		m_timeScale = timeScale;
	}

	void ParticleSimulationStepKernel::Task::SetTimeMode(TimeMode timeMode) {
		m_timeMode = static_cast<uint32_t>(timeMode);
	}

	void ParticleSimulationStepKernel::Task::Synchronize() {
		const Reference<ParticleBuffers> buffers = [&]() ->Reference<ParticleBuffers> {
			std::unique_lock<SpinLock> lock(m_bufferLock);
			Reference<ParticleBuffers> rv = m_buffers;
			return rv;
		}();
		if (m_lastBuffers != buffers) {
			if (buffers != nullptr) {
				m_particleStateBuffer = buffers->GetBuffer(ParticleState::BufferId());
				if (m_particleStateBuffer == nullptr)
					Context()->Log()->Error("ParticleSimulationStepKernel::Task::Synchronize - Failed to get ParticleState buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}
			else m_particleStateBuffer = nullptr;
			m_lastBuffers = buffers;
		}
		Helpers::ParticleTaskSettings settings = {};
		{
			if (m_particleStateBuffer != nullptr) {
				settings.particleStateBufferId = m_particleStateBuffer->Index();
				settings.particleCount = static_cast<uint32_t>(m_lastBuffers->ParticleBudget());
			}
			{
				settings.timeScale = m_timeScale.load();
				settings.timeType = m_timeMode.load();
			}
		}
		SetSettings(settings);
	}


	ParticleSimulationStepKernel::ParticleSimulationStepKernel() : ParticleKernel(sizeof(Helpers::ParticleTaskSettings)) {}
	ParticleSimulationStepKernel::~ParticleSimulationStepKernel() {}

	Reference<ParticleKernel::Instance> ParticleSimulationStepKernel::CreateInstance(SceneContext* context)const {
		if (context == nullptr) return nullptr;

		const Graphics::BufferReference<Vector4> timeInfo = context->Graphics()->Device()->CreateConstantBuffer<Vector4>();
		if (timeInfo == nullptr) {
			context->Log()->Error("ParticleSimulationStepKernel::CreateInstance - Failed to create time buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}

		static const Graphics::ShaderClass SHADER_CLASS("Jimara/Environment/Rendering/Particles/Kernels/SimulationStep/ParticleSimulationStepKernel");
		const Helpers::BindingSet bindingSet(context, timeInfo);
		const Reference<ParticleKernel::Instance> kernelInstance = CombinedParticleKernel<Helpers::ParticleTaskSettings>::Create(context, &SHADER_CLASS, bindingSet);
		if (kernelInstance == nullptr) {
			context->Log()->Error("ParticleSimulationStepKernel::CreateInstance - Failed to create combined kernel instance! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}

		return Object::Instantiate<Helpers::KernelInstance>(context, timeInfo, kernelInstance);
	}
}

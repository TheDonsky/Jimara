#include "InstanceBufferGenerator.h"


namespace Jimara {
	struct ParticleInstanceBufferGenerator::Helpers {
		struct TaskDescriptor : TaskSettings {
			alignas(4) uint32_t startThreadIndex = 0u;		// Bytes [80 - 84)
			alignas(4) uint32_t endThreadIndex = 0u;		// Bytes [84 - 88)
			alignas(4) uint32_t padA = 0u, padB = 0u;		// Bytes [88 - 96)
		};

		static_assert(sizeof(TaskDescriptor) == 96u);
		static_assert(offsetof(TaskDescriptor, startThreadIndex) == 80u);

		class KernelInstance : public virtual ParticleKernel::Instance {
		private:
			const Reference<SceneContext> m_context;

		public:
			inline KernelInstance(SceneContext* context) : m_context(context) {

			}

			inline virtual void Execute(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, const Task* const* tasks, size_t taskCount)const override {
				// __TODO__: Implement this crap!
				m_context->Log()->Error("ParticleInstanceBufferGenerator::Helpers::KernelInstance::Execute - Not yet implemented!");
			}
		};
	};

	ParticleInstanceBufferGenerator::ParticleInstanceBufferGenerator() 
		: ParticleKernel(sizeof(TaskSettings)) {}

	ParticleInstanceBufferGenerator::~ParticleInstanceBufferGenerator() {}

	const ParticleInstanceBufferGenerator* ParticleInstanceBufferGenerator::Instance() {
		static const ParticleInstanceBufferGenerator instance;
		return &instance;
	}

	Reference<ParticleKernel::Instance> ParticleInstanceBufferGenerator::CreateInstance(SceneContext* context)const {
		if (context == nullptr) return nullptr;
		else return Object::Instantiate<Helpers::KernelInstance>(context);
	}
}

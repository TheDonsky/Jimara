#include "InstanceBufferGenerator.h"
#include "../CombinedParticleKernel.h"


namespace Jimara {
	ParticleInstanceBufferGenerator::ParticleInstanceBufferGenerator() 
		: ParticleKernel(sizeof(TaskSettings)) {}

	ParticleInstanceBufferGenerator::~ParticleInstanceBufferGenerator() {}

	const ParticleInstanceBufferGenerator* ParticleInstanceBufferGenerator::Instance() {
		static const ParticleInstanceBufferGenerator instance;
		return &instance;
	}

	Reference<ParticleKernel::Instance> ParticleInstanceBufferGenerator::CreateInstance(SceneContext* context)const {
		const Graphics::ShaderClass SHADER_CLASS("Jimara/Environment/Rendering/Particles/Kernels/InstanceBufferGenerator/InstanceBufferGenerator_Kernel");
		return CombinedParticleKernel<TaskSettings>::Create(context, &SHADER_CLASS);
	}
}

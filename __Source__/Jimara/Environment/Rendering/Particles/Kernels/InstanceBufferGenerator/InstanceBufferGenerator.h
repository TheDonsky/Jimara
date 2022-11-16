#pragma once
#include "../../ParticleKernel.h"


namespace Jimara {
	class JIMARA_API ParticleInstanceBufferGenerator : public virtual ParticleKernel {
	public:
		struct TaskSettings {
			alignas(4) uint32_t particleStateBufferId = 0u;			// Bytes [0 - 4)
			alignas(4) uint32_t instanceBufferId = 0u;				// Bytes [0 - 8)
			alignas(4) uint32_t instanceStartId = 0u;				// Bytes [0 - 12)
			alignas(4) uint32_t instanceEndId = 0u;					// Bytes [0 - 16)
			alignas(16) Matrix4 baseTransform = Math::Identity();	// Bytes [16 - 80)
		};
		static_assert(sizeof(TaskSettings) == 80);

		virtual ~ParticleInstanceBufferGenerator();

		static const ParticleInstanceBufferGenerator* Instance();

		virtual Reference<ParticleKernel::Instance> CreateInstance(SceneContext* context)const override;

	private:
		ParticleInstanceBufferGenerator();

		struct Helpers;
	};
}

#pragma once
#include "../../Scene/Scene.h"
#include "ParticleKernel.h"

namespace Jimara {
	class JIMARA_API ParticleSimulation {
	public:
		typedef ParticleKernel::Task Task;

		typedef Reference<Task, ParticleSimulation> TaskBinding;

	private:
		inline ParticleSimulation();
		inline virtual ~ParticleSimulation() {}

		struct Helpers;

		static void AddReference(Task* task);
		static void ReleaseReference(Task* task);
	};
}

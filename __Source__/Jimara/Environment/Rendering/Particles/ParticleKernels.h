#pragma once
#include "../../GraphicsSimulation/GraphicsSimulation.h"


namespace Jimara {
	class JIMARA_API ParticleInitializationKernel : public virtual GraphicsSimulation::Kernel {
	public:
		class JIMARA_API Task : public virtual GraphicsSimulation::Task {
		public:
			// __TODO__: Define this crap!

		private:

		};

		virtual Reference<Task> CreateTask(SceneContext* context)const = 0;
	};

	class JIMARA_API ParticleSimulationKernel : public virtual GraphicsSimulation::Kernel {
	public:
		class JIMARA_API Task : public virtual GraphicsSimulation::Task {
		public:
			// __TODO__: Define this crap!

		private:

		};

		virtual Reference<Task> CreateTask(SceneContext* context)const = 0;
	};
}

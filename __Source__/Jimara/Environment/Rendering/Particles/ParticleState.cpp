#include "ParticleState.h"


namespace Jimara {
	const ParticleBuffers::BufferId* ParticleState::BufferId() {
		static const Reference<const ParticleBuffers::BufferId> BUFFER_ID = ParticleBuffers::BufferId::Create<ParticleState>("ParticleState");
		return BUFFER_ID;
	}
}

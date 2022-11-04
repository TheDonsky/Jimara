#pragma once
#include "ParticleBuffers.h"


namespace Jimara {
	struct ParticleState {
		alignas(16) Vector3 position = Vector3(0.0f);			// Bytes [0 - 12)
		alignas(16) Vector3 eulerAngles = Vector3(0.0f);		// Bytes [16 - 28)
		alignas(16) Vector3 size = Vector3(1.0f);				// Bytes [32 - 44)

		alignas(16) Vector3 velocity = Vector3(0.0f);			// Bytes [48 - 60)
		alignas(4) float totalLifetime = 1.0f;					// Bytes [60 - 64)
		alignas(16) Vector3 angularVelocity = Vector3(0.0f);	// Bytes [64 - 76)
		alignas(4) float remainingLifetime = 0.0f;				// Bytes [76 - 80)

		alignas(16) Vector4 color = Vector4(1.0f);				// Bytes [80 - 96)

		static const ParticleBuffers::BufferId* BufferId();
	};

	static_assert(sizeof(ParticleState) == 96);
}

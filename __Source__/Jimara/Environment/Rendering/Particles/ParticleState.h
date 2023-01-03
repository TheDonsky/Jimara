#pragma once
#include "ParticleBuffers.h"


namespace Jimara {
	/// <summary>
	/// Element within the primary state buffer for each Particle System
	/// </summary>
	struct JIMARA_API ParticleState {
		/// <summary> Position of an individual particle </summary>
		alignas(16) Vector3 position = Vector3(0.0f);					// Bytes [0 - 12)

		/// <summary> Currently remaining lifetime before the specific particle disapears </summary>
		alignas(4) float remainingLifetime = 1.0f;						// Bytes [12 - 16)

		/// <summary> Spawn-time position of an individual particle </summary>
		alignas(16) Vector3 initialPosition = Vector3(0.0f);			// Bytes [16 - 28)

		/// <summary> Total lifetime at the moment of spawning </summary>
		alignas(4) float totalLifetime = 1.0f;							// Bytes [28 - 32)


		/// <summary> YXZ Euler angles for an individual particle (for view-facing quads, only Z should be updated) </summary>
		alignas(16) Vector3 eulerAngles = Vector3(0.0f);				// Bytes [32 - 44)

		/// <summary> Spawn-time YXZ Euler angles for an individual particle (for view-facing quads, only Z should be updated) </summary>
		alignas(16) Vector3 initialEulerAngles = Vector3(0.0f);			// Bytes [48 - 60)


		/// <summary> Size/Scale of an individual particle </summary>
		alignas(16) Vector3 size = Vector3(1.0f);						// Bytes [64 - 76)

		/// <summary> Spawn-time size/scale of an individual particle </summary>
		alignas(16) Vector3 initialSize = Vector3(1.0f);				// Bytes [80 - 92)


		/// <summary> Current velocity of the particle (dictates how the position changes) </summary>
		alignas(16) Vector3 velocity = Vector3(0.0f);					// Bytes [96 - 108)

		/// <summary> Initial velocity of the particle (dictates how the position changes) </summary>
		alignas(16) Vector3 initialVelocity = Vector3(0.0f);			// Bytes [112 - 124)


		/// <summary> Current angular velocity of the particle (dictates how the eulerAngles change) </summary>
		alignas(16) Vector3 angularVelocity = Vector3(0.0f);			// Bytes [128 - 140)

		/// <summary> Initial angular velocity of the particle (dictates how the eulerAngles change) </summary>
		alignas(16) Vector3 initialAngularVelocity = Vector3(0.0f);		// Bytes [144 - 156)


		/// <summary> Color of the particle </summary>
		alignas(16) Vector4 color = Vector4(1.0f);						// Bytes [160 - 176)

		/// <summary> Initial Color of the particle </summary>
		alignas(16) Vector4 initialColor = Vector4(1.0f);				// Bytes [176 - 192)


		/// <summary> Unique ParticleState buffer identifier per ParticleBuffers object </summary>
		static const ParticleBuffers::BufferId* BufferId();
	};

	static_assert(sizeof(ParticleState) == 192);
}

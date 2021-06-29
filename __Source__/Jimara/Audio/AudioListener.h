#pragma once
#include "../Core/Object.h"
#include "../Math/Math.h"


namespace Jimara {
	namespace Audio {
		class AudioListener : public virtual Object {
		public:
			struct Settings {
				/// <summary> World space position and rotation (scale not supported) </summary>
				Matrix4 pose = Math::Identity();

				/// <summary> World space movement speed </summary>
				Vector3 velocity = Vector3(0.0f, 0.0f, 0.0f);

				/// <summary> Source volume multiplier </summary>
				float volume = 1.0f;
			};

			virtual void Update(const Settings& newSettings) = 0;
		};
	}
}

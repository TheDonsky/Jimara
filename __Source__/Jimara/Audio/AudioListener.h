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

				/// <summary> Source volume multiplier </summary>
				float volume = 1.0f;
			};

			virtual void Update(const Settings& newSettings) = 0;
		};
	}
}

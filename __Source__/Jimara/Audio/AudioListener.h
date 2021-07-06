#pragma once
#include "../Core/Object.h"
#include "../Math/Math.h"


namespace Jimara {
	namespace Audio {
		/// <summary>
		/// A listener an ear, a mike, an object that captures all the sounds emitted by the AudioSources
		/// </summary>
		class AudioListener : public virtual Object {
		public:
			/// <summary>
			/// Listener settings
			/// </summary>
			struct Settings {
				/// <summary> World space position and rotation (scale not supported) </summary>
				Matrix4 pose = Math::Identity();

				/// <summary> World space movement speed </summary>
				Vector3 velocity = Vector3(0.0f, 0.0f, 0.0f);

				/// <summary> Source volume multiplier </summary>
				float volume = 1.0f;

				/// <summary>
				/// Compares to other settiongs
				/// </summary>
				/// <param name="other"> Settings to compare to </param>
				/// <returns> True, if the settings are equal </returns>
				inline bool operator==(const Settings& other)const { return pose == other.pose && velocity == other.velocity && volume == other.volume; }
			};

			/// <summary>
			/// Updates listener settings
			/// </summary>
			/// <param name="newSettings"> New settings to set </param>
			virtual void Update(const Settings& newSettings) = 0;
		};
	}
}

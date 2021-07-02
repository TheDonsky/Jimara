#pragma once
#include "AudioClip.h"
#include "../Math/Math.h"


namespace Jimara {
	namespace Audio {
		class AudioSource : public virtual Object {
		public:
			enum class PlaybackState : uint8_t {
				PLAYING,
				PAUSED,
				STOPPED
			};

			virtual int Priority()const = 0;

			virtual void SetPriority(int priority) = 0;

			virtual PlaybackState State()const = 0;

			virtual void Play() = 0;

			virtual void Pause() = 0;

			virtual void Stop() = 0;

			virtual float Time()const = 0;

			virtual float SetTime(float time) = 0;

			virtual bool Looping()const = 0;

			virtual void SetLooping(bool loop) = 0;

			virtual AudioClip* Clip()const = 0;

			virtual void SetClip(AudioClip* clip, bool resetTime = false) = 0;
		};

		class AudioSource2D : public virtual AudioSource {
		public:
			struct Settings {
				/// <summary> Source volume </summary>
				float volume = 1.0f;

				/// <summary> Playback speed </summary>
				float pitch = 1.0f;
			};

			virtual void Update(const Settings& newSettings) = 0;
		};

		class AudioSource3D : public virtual Object {
		public:
			struct Settings {
				/// <summary> World space position </summary>
				Vector3 position = Vector3(0.0f, 0.0f, 0.0f);

				/// <summary> World space movement speed </summary>
				Vector3 velocity = Vector3(0.0f, 0.0f, 0.0f);

				/// <summary> Source volume </summary>
				float volume = 1.0f;

				/// <summary> Playback speed </summary>
				float pitch = 1.0f;
			};

			virtual void Update(const Settings& newSettings) = 0;
		};
	}
}

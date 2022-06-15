#pragma once
#include "AudioClip.h"
#include "../Math/Math.h"


namespace Jimara {
	namespace Audio {
		/// <summary>
		/// Audio source/sound emitter
		/// </summary>
		class JIMARA_API AudioSource : public virtual Object {
		public:
			/// <summary>
			/// Source playback state
			/// </summary>
			enum class PlaybackState : uint8_t {
				/// <summary> Source is currently playing a clip </summary>
				PLAYING,

				/// <summary> Source playback has been manually paused </summary>
				PAUSED,

				/// <summary> Source playback has been manually stopped </summary>
				STOPPED,

				/// <summary> Source has finished playback on it's own terms </summary>
				FINISHED
			};

			/// <summary> Source priority (in case there are some limitations about the number of actively playing sounds on the underlying hardware, higherst priority ones will be heared) </summary>
			virtual int Priority()const = 0;

			/// <summary>
			/// Updates source priority
			/// (in case there are some limitations about the number of actively playing sounds on the underlying hardware, higherst priority ones will be heared)
			/// </summary>
			/// <param name="priority"> New priority to set </param>
			virtual void SetPriority(int priority) = 0;

			/// <summary> Current source playback state </summary>
			virtual PlaybackState State()const = 0;

			/// <summary> Starts/Resumes/Restarts playback </summary>
			virtual void Play() = 0;

			/// <summary> Interrupts playback and saves time till the next Play() command </summary>
			virtual void Pause() = 0;

			/// <summary> Stops playback and resets time </summary>
			virtual void Stop() = 0;

			/// <summary> Time (in seconds) since the beginning of the Clip </summary>
			virtual float Time()const = 0;

			/// <summary>
			/// Sets clip time offset
			/// </summary>
			/// <param name="time"> Time offset </param>
			virtual void SetTime(float time) = 0;

			/// <summary> If true, playback will keep looping untill paused/stopped or made non-looping </summary>
			virtual bool Looping()const = 0;

			/// <summary>
			/// Makes the source looping or non-looping
			/// </summary>
			/// <param name="loop"> If true, the source will keep looping untill paused/stopped or made non-looping when played </param>
			virtual void SetLooping(bool loop) = 0;

			/// <summary> AudioClip, tied to the source </summary>
			virtual AudioClip* Clip()const = 0;

			/// <summary>
			/// Sets audioClip
			/// </summary>
			/// <param name="clip"> AudioClip to play </param>
			/// <param name="resetTime"> If true and the source is playing or paused, time offset will be preserved </param>
			virtual void SetClip(AudioClip* clip, bool resetTime = false) = 0;
		};

		/// <summary>
		/// 2D/Flat/background sound emitter
		/// </summary>
		class JIMARA_API AudioSource2D : public virtual AudioSource {
		public:
			/// <summary>
			/// 2D source settings
			/// </summary>
			struct Settings {
				/// <summary> Source volume </summary>
				float volume = 1.0f;

				/// <summary> Playback speed </summary>
				float pitch = 1.0f;

				/// <summary>
				/// Compares to other settiongs
				/// </summary>
				/// <param name="other"> Settings to compare to </param>
				/// <returns> True, if the settings are equal </returns>
				inline bool operator==(const Settings& other)const { return volume == other.volume && pitch == other.pitch; }
			};

			/// <summary>
			/// Updates source settings
			/// </summary>
			/// <param name="newSettings"> New settings to use </param>
			virtual void Update(const Settings& newSettings) = 0;
		};

		/// <summary>
		/// 3D/Posed sound emitter
		/// </summary>
		class JIMARA_API AudioSource3D : public virtual AudioSource {
		public:
			/// <summary>
			/// 3D source settings
			/// </summary>
			struct Settings {
				/// <summary> World space position </summary>
				Vector3 position = Vector3(0.0f, 0.0f, 0.0f);

				/// <summary> World space movement speed </summary>
				Vector3 velocity = Vector3(0.0f, 0.0f, 0.0f);

				/// <summary> Source volume </summary>
				float volume = 1.0f;

				/// <summary> Playback speed </summary>
				float pitch = 1.0f;

				/// <summary>
				/// Compares to other settiongs
				/// </summary>
				/// <param name="other"> Settings to compare to </param>
				/// <returns> True, if the settings are equal </returns>
				inline bool operator==(const Settings& other)const { return position == other.position && velocity == other.velocity && volume == other.volume && pitch == other.pitch; }
			};

			/// <summary>
			/// Updates source settings
			/// </summary>
			/// <param name="newSettings"> New settings to use </param>
			virtual void Update(const Settings& newSettings) = 0;
		};
	}
}

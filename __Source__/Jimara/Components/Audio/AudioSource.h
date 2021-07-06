#pragma once
#include "../Component.h"
#include "../Interfaces/Updatable.h"


namespace Jimara {
	/// <summary>
	/// AudioSource component
	/// </summary>
	class AudioSource : public virtual Component {
	public:
		/// <summary> Source volume </summary>
		float Volume()const;

		/// <summary>
		/// Updates source volume
		/// </summary>
		/// <param name="volume"> New volume to use </param>
		void SetVolume(float volume);

		/// <summary> Playback speed </summary>
		float Pitch()const;

		/// <summary>
		/// Updates source playback speed
		/// </summary>
		/// <param name="pitch"> New pitch to use </param>
		void SetPitch(float pitch);

		/// <summary> Source priority (in case there are some limitations about the number of actively playing sounds on the underlying hardware, higherst priority ones will be heared) </summary>
		int Priority()const;

		/// <summary>
		/// Updates source priority
		/// (in case there are some limitations about the number of actively playing sounds on the underlying hardware, higherst priority ones will be heared)
		/// </summary>
		/// <param name="priority"> New priority to set </param>
		void SetPriority(int priority);

		/// <summary> If true, playback will keep looping untill paused/stopped or made non-looping </summary>
		bool Looping()const;

		/// <summary>
		/// Makes the source looping or non-looping
		/// </summary>
		/// <param name="loop"> If true, the source will keep looping untill paused/stopped or made non-looping when played </param>
		void SetLooping(bool looping);

		/// <summary> AudioClip, tied to the source </summary>
		Audio::AudioClip* Clip()const;

		/// <summary>
		/// Sets audioClip (does not preserve time if already playing)
		/// </summary>
		/// <param name="clip"> AudioClip to play </param>
		void SetClip(Audio::AudioClip* clip);

		/// <summary> True, if the main clip, associated with this source is playing </summary>
		bool Playing()const;

		/// <summary> Starts/Resumes/Restarts playback </summary>
		void Play();

		/// <summary> Sets new clip and starts/resumes/restarts playback </summary>
		void Play(Audio::AudioClip* clip);

		/// <summary> Interrupts playback and saves time till the next Play() command </summary>
		void Pause();

		/// <summary> Stops playback and resets time </summary>
		void Stop();

		/// <summary>
		/// Plays a one-time audio without altering the current playback state
		/// </summary>
		/// <param name="clip"> Clip to play </param>
		virtual void PlayOneShot(Audio::AudioClip* clip) = 0;

	protected:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="source"> Toolbox source </param>
		/// <param name="volume"> Initial volume </param>
		/// <param name="pitch"> Initial pitch </param>
		AudioSource(Reference<Audio::AudioSource> source, float volume, float pitch);

		/// <summary> Main toolbox source </summary>
		Audio::AudioSource* Source()const;

	private:
		// Main toolbox source
		const Reference<Audio::AudioSource> m_source;

		// Current volume
		float m_volume = 0.0f;

		// Playback speed
		float m_pitch = 0.0f;
	};



	/// <summary>
	/// 2D/Non-Posed/Background audio emitter component
	/// </summary>
	class AudioSource2D : public virtual AudioSource, public virtual Updatable {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		/// <param name="clip"> Initial AudioClip to tie to the source </param>
		/// <param name="volume"> Initial volume </param>
		/// <param name="pitch"> Initial pitch </param>
		AudioSource2D(Component* parent, const std::string_view& name, Audio::AudioClip* clip = nullptr, float volume = 1.0f, float pitch = 1.0f);

		/// <summary>
		/// Plays a one-time audio without altering the current playback state
		/// </summary>
		/// <param name="clip"> Clip to play </param>
		virtual void PlayOneShot(Audio::AudioClip* clip) override;

		/// <summary> Invoked each time the logical scene is updated (synchs scne with AudioScene) </summary>
		virtual void Update()override;

	private:
		// Lock for PlayOneShot()
		std::mutex m_lock;

		// Settings from last Update cycle
		Audio::AudioSource2D::Settings m_settings;

		// Set of all active sources from PlayOneShot() calls
		std::unordered_set<Reference<Audio::AudioSource2D>> m_oneShotSources;
	};


	/// <summary>
	/// 3D/Posed/World-Space audio emitter component
	/// </summary>
	class AudioSource3D : public virtual AudioSource, public virtual Updatable {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		/// <param name="clip"> Initial AudioClip to tie to the source </param>
		/// <param name="volume"> Initial volume </param>
		/// <param name="pitch"> Initial pitch </param>
		AudioSource3D(Component* parent, const std::string_view& name, Audio::AudioClip* clip = nullptr, float volume = 1.0f, float pitch = 1.0f);

		/// <summary>
		/// Plays a one-time audio without altering the current playback state
		/// </summary>
		/// <param name="clip"> Clip to play </param>
		virtual void PlayOneShot(Audio::AudioClip* clip) override;

		/// <summary> Invoked each time the logical scene is updated (synchs scne with AudioScene) </summary>
		virtual void Update()override;

	private:
		// Lock for PlayOneShot()
		std::mutex m_lock;

		// Settings from last Update cycle
		Audio::AudioSource3D::Settings m_settings;

		// Set of all active sources from PlayOneShot() calls
		std::unordered_set<Reference<Audio::AudioSource3D>> m_oneShotSources;
	};
}

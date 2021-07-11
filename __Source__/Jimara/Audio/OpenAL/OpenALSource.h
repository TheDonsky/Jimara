#pragma once
#include "../../Core/Object.h"
namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALSource;
			class SourcePlayback;
			class SourcePlayback2D;
			class SourcePlayback3D;
		}
	}
}
#include "OpenALClip.h"
#include "OpenALScene.h"
#include <optional>
#include <map>


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			/// <summary>
			/// OpenAL-backed AudioSource
			/// </summary>
			class OpenALSource : public virtual AudioSource {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="scene"> Scene, the source resides on </param>
				/// <param name="clip"> Initial AudioClip, the source will use </param>
				OpenALSource(OpenALScene* scene, OpenALClip* clip);

				/// <summary> Virtual destructor </summary>
				virtual ~OpenALSource();

				/// <summary> Source priority (in case there are some limitations about the number of actively playing sounds on the underlying hardware, higherst priority ones will be heared) </summary>
				virtual int Priority()const override;

				/// <summary>
				/// Updates source priority
				/// (in case there are some limitations about the number of actively playing sounds on the underlying hardware, higherst priority ones will be heared)
				/// </summary>
				/// <param name="priority"> New priority to set </param>
				virtual void SetPriority(int priority) override;

				/// <summary> Current source playback state </summary>
				virtual PlaybackState State()const override;

				/// <summary> Starts/Resumes/Restarts playback </summary>
				virtual void Play() override;

				/// <summary> Interrupts playback and saves time till the next Play() command </summary>
				virtual void Pause() override;

				/// <summary> Stops playback and resets time </summary>
				virtual void Stop() override;

				/// <summary> Time (in seconds) since the beginning of the Clip </summary>
				virtual float Time()const override;

				/// <summary>
				/// Sets clip time offset
				/// </summary>
				/// <param name="time"> Time offset </param>
				virtual void SetTime(float time) override;

				/// <summary> If true, playback will keep looping untill paused/stopped or made non-looping </summary>
				virtual bool Looping()const override;

				/// <summary>
				/// Makes the source looping or non-looping
				/// </summary>
				/// <param name="loop"> If true, the source will keep looping untill paused/stopped or made non-looping when played </param>
				virtual void SetLooping(bool loop) override;

				/// <summary> AudioClip, tied to the source </summary>
				virtual AudioClip* Clip()const override;

				/// <summary>
				/// Sets audioClip
				/// </summary>
				/// <param name="clip"> AudioClip to play </param>
				/// <param name="resetTime"> If true and the source is playing or paused, time offset will be preserved </param>
				virtual void SetClip(AudioClip* clip, bool resetTime = false) override;

			protected:
				/// <summary>
				/// Creates a new SourcePlayback instance
				/// </summary>
				/// <param name="clip"> AudioCLip to use </param>
				/// <param name="timeOffset"> Initial time offset for the playback </param>
				/// <param name="looping"> If true, the playback should be looping </param>
				/// <returns> New instance of a source playback </returns>
				virtual Reference<SourcePlayback> BeginPlayback(OpenALClip* clip, float timeOffset, bool looping) = 0;

				/// <summary>
				/// Sets source pitch (for correct time calculations)
				/// </summary>
				/// <param name="pitch"> New playback speed </param>
				void SetPitch(float pitch);

				/// <summary> Current active playback (can be null) </summary>
				Reference<SourcePlayback> Playback()const;

				/// <summary> Internal state lock </summary>
				std::mutex& Lock()const;

			private:
				// Scene, the source resides on
				const Reference<OpenALScene> m_scene;

				// Object, that separates lock allocation from the source and makes destruction safer
				struct LockInstance : public virtual Object {
					// Internal state lock
					mutable std::mutex lock;

					// Owner of the object
					OpenALSource* owner = nullptr;
				};

				// Internal state lock
				Reference<LockInstance> m_lock;

				// Current source priority
				std::atomic<int> m_priority = 0;

				// If true, the source will loop during playback
				std::atomic<bool> m_looping = false;

				// Stores data about initial time offset if the playback starts
				std::optional<std::atomic<float>> m_time;

				// Current playback speed
				std::atomic<float> m_pitch = 1.0f;

				// AudioClip, used by the source
				Reference<OpenALClip> m_clip;

				// Current active playback (can be null)
				Reference<SourcePlayback> m_playback;

				// Invoked on each instance tick when playing
				void OnTick(float deltaTime, ActionQueue<>& queue);
			};



#pragma warning(disable: 4250)
			/// <summary>
			/// OpenAL-backed 2d source
			/// </summary>
			class OpenALSource2D : public virtual AudioSource2D, public virtual OpenALSource {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="scene"> Scene, the source resides on </param>
				/// <param name="clip"> Initial AudioClip, the source will use </param>
				/// <param name="settings"> Initial source settings </param>
				OpenALSource2D(OpenALScene* scene, OpenALClip* clip, const AudioSource2D::Settings& settings);

				/// <summary>
				/// Updates source settings
				/// </summary>
				/// <param name="newSettings"> New settings to use </param>
				virtual void Update(const Settings& newSettings) override;

			protected:
				/// <summary>
				/// Creates a new SourcePlayback instance
				/// </summary>
				/// <param name="clip"> AudioCLip to use </param>
				/// <param name="timeOffset"> Initial time offset for the playback </param>
				/// <param name="looping"> If true, the playback will be looping </param>
				/// <returns> New instance of a source playback </returns>
				virtual Reference<SourcePlayback> BeginPlayback(OpenALClip* clip, float timeOffset, bool looping) override;

			private:
				// Current settings
				AudioSource2D::Settings m_settings;
			};

			/// <summary>
			/// OpenAL-backed 3d source
			/// </summary>
			class OpenALSource3D : public virtual AudioSource3D, public virtual OpenALSource {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="scene"> Scene, the source resides on </param>
				/// <param name="clip"> Initial AudioClip, the source will use </param>
				/// <param name="settings"> Initial source settings </param>
				OpenALSource3D(OpenALScene* scene, OpenALClip* clip, const AudioSource3D::Settings& settings);

				/// <summary>
				/// Updates source settings
				/// </summary>
				/// <param name="newSettings"> New settings to use </param>
				virtual void Update(const Settings& newSettings) override;

			protected:
				/// <summary>
				/// Creates a new SourcePlayback instance
				/// </summary>
				/// <param name="clip"> AudioCLip to use </param>
				/// <param name="timeOffset"> Initial time offset for the playback </param>
				/// <param name="looping"> If true, the playback will be looping </param>
				/// <returns> New instance of a source playback </returns>
				virtual Reference<SourcePlayback> BeginPlayback(OpenALClip* clip, float timeOffset, bool looping) override;

			private:
				// Current settings
				AudioSource3D::Settings m_settings;
			};

#pragma warning(default: 4250)


			/// <summary>
			/// Whenever a source is playing, it creates an instance of a playback and adds it to the scene, removing it when stopped or finished
			/// </summary>
			class SourcePlayback : public virtual Object {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="clip"> Audio clip to play </param>
				/// <param name="timeOffset"> Playback time offset </param>
				/// <param name="looping"> If true, the playback will be looping </param>
				SourcePlayback(OpenALClip* clip, float timeOffset, bool looping);

				/// <summary> AudioClip, the playback is tied to </summary>
				OpenALClip* Clip()const;

				/// <summary>
				/// Adds a listener to the playback (creates a ClipPlayback for each listener added)
				/// </summary>
				/// <param name="context"> Listener context to add </param>
				virtual void AddListener(ListenerContext* context) = 0;

				/// <summary>
				/// Removes a listener to the playback (removes a ClipPlayback for each listener added)
				/// </summary>
				/// <param name="context"> Listener context to remove </param>
				virtual void RemoveListener(ListenerContext* context) = 0;

				/// <summary> Removes all currently added listeners </summary>
				virtual void RemoveAllListeners() = 0;

				/// <summary> True, if the playback time has not yet reached the Clip's duration, playback is looping or any of the ClipPlaybacks are still playing </summary>
				virtual bool Playing();

				/// <summary> If true, playback will keep looping untill paused/stopped or made non-looping </summary>
				bool Looping()const;

				/// <summary>
				/// Makes the playback looping or non-looping
				/// </summary>
				/// <param name="loop"> If true, the playback will keep looping untill stopped or made non-looping </param>
				virtual void SetLooping(bool looping);

				/// <summary> Current time offset </summary>
				float Time()const;

			private:
				// Clip to play
				const Reference<OpenALClip> m_clip;

				// Internal playback timer
				std::atomic<float> m_time;

				// True, when the playback is looping
				std::atomic<bool> m_looping;

				// Advances playback time
				void AdvanceTime(float deltaTime);

				// AdvanceTime() is invoked by OpenALSource
				friend class OpenALSource;
			};


			/// <summary>
			/// Concrete implementation of SourcePlayback
			/// </summary>
			/// <typeparam name="ClipPlaybackType"> Type of the ClipPlaybacks to store (2d/3d) </typeparam>
			/// <typeparam name="SourceSettings"> Source settings type (2d/3d) </typeparam>
			template<typename SourceSettings, typename ClipPlaybackType>
			class SourcePlaybackWithClipPlaybacks : public SourcePlayback {
			private:
				// Playbacks per listener
				typedef std::map<Reference<ListenerContext>, Reference<ClipPlaybackType>> Playbacks;

				// Lock for internals
				std::mutex m_listenerLock;

				// Playbacks per listener
				Playbacks m_playbacks;

				// Current settings
				SourceSettings m_settings;

				// Current pitch
				std::atomic<float> m_pitch;


			protected:
				/// <summary> Current pitch </summary>
				inline float Pitch()const { return m_pitch; }

				/// <summary>
				/// Creates a new ClipPlayback instance
				/// </summary>
				/// <param name="settings"> Source settings </param>
				/// <param name="context"> Listener </param>
				/// <param name="looping"> True, if the playback is currently looping </param>
				/// <param name="timeOffset"> Initial time offset for the clip playback </param>
				/// <returns> New instance of the clip playback </returns>
				virtual Reference<ClipPlaybackType> BeginClipPlayback(const SourceSettings& settings, ListenerContext* context, bool looping, float timeOffset) = 0;


			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="clip"> AudioClip to play </param>
				/// <param name="timeOffset"> Initial time offset for the playback </param>
				/// <param name="looping"> True, if the playback should be looping </param>
				/// <param name="settings"> Initial source settings </param>
				inline SourcePlaybackWithClipPlaybacks(OpenALClip* clip, float timeOffset, bool looping, const SourceSettings& settings) 
					: SourcePlayback(clip, timeOffset, looping), m_settings(settings), m_pitch(settings.pitch) {}

				/// <summary>
				/// Updates source settings
				/// </summary>
				/// <param name="settings"> New settings to set </param>
				inline void Update(const SourceSettings& settings) {
					std::unique_lock<std::mutex> lock(m_listenerLock);
					m_settings = settings;
					m_pitch = settings.pitch;
					for (typename Playbacks::const_iterator it = m_playbacks.begin(); it != m_playbacks.end(); ++it)
						it->second->Update(settings);
				}

				/// <summary>
				/// Adds a listener to the playback (creates a ClipPlayback for each listener added)
				/// </summary>
				/// <param name="context"> Listener context to add </param>
				inline virtual void AddListener(ListenerContext* context) override {
					if (context == nullptr) return;
					std::unique_lock<std::mutex> lock(m_listenerLock);
#ifndef NDEBUG
					typename Playbacks::const_iterator it = m_playbacks.find(context);
					if (it != m_playbacks.end()) {
						context->Device()->APIInstance()->Log()->Warning("SourcePlaybackWithClipPlaybacks::AddListener - context already included!");
						return;
					}
#endif
					m_playbacks[context] = BeginClipPlayback(m_settings, context, Looping(), Time());
				}

				/// <summary>
				/// Removes a listener to the playback (removes a ClipPlayback for each listener added)
				/// </summary>
				/// <param name="context"> Listener context to remove </param>
				inline virtual void RemoveListener(ListenerContext* context) override {
					if (context == nullptr) return;
					std::unique_lock<std::mutex> lock(m_listenerLock);
					m_playbacks.erase(context);
				}

				/// <summary> Removes all currently added listeners </summary>
				inline virtual void RemoveAllListeners() override {
					std::unique_lock<std::mutex> lock(m_listenerLock);
					m_playbacks.clear();
				}

				/// <summary> True, if the playback time has not yet reached the Clip's duration, playback is looping or any of the ClipPlaybacks are still playing </summary>
				virtual bool Playing()override {
					if (SourcePlayback::Playing()) return true;
					std::unique_lock<std::mutex> lock(m_listenerLock);
					for (typename Playbacks::const_iterator it = m_playbacks.begin(); it != m_playbacks.end(); ++it)
						if (it->second->Playing()) return true;
					return false;
				}

				/// <summary>
				/// Makes the playback looping or non-looping
				/// </summary>
				/// <param name="loop"> If true, the playback will keep looping untill stopped or made non-looping </param>
				virtual void SetLooping(bool looping)override {
					std::unique_lock<std::mutex> lock(m_listenerLock);
					SourcePlayback::SetLooping(looping);
					for (typename Playbacks::const_iterator it = m_playbacks.begin(); it != m_playbacks.end(); ++it)
						it->second->Loop(looping);
				}
			};

			/// <summary>
			/// 2D source playback
			/// </summary>
			class SourcePlayback2D : public SourcePlaybackWithClipPlaybacks<AudioSource2D::Settings, ClipPlayback2D> {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="clip"> AudioClip to play </param>
				/// <param name="timeOffset"> Initial time offset for the playback </param>
				/// <param name="looping"> True, if the playback should be looping </param>
				/// <param name="settings"> Initial source settings </param>
				SourcePlayback2D(OpenALClip* clip, float timeOffset, bool looping, const AudioSource2D::Settings& settings);

			protected:
				/// <summary>
				/// Creates a new ClipPlayback instance
				/// </summary>
				/// <param name="settings"> Source settings </param>
				/// <param name="context"> Listener </param>
				/// <param name="looping"> True, if the playback is currently looping </param>
				/// <param name="timeOffset"> Initial time offset for the clip playback </param>
				/// <returns> New instance of the clip playback </returns>
				virtual Reference<ClipPlayback2D> BeginClipPlayback(const AudioSource2D::Settings& settings, ListenerContext* context, bool looping, float timeOffset) override;
			};

			/// <summary>
			/// 3D source playback
			/// </summary>
			class SourcePlayback3D : public SourcePlaybackWithClipPlaybacks<AudioSource3D::Settings, ClipPlayback3D> {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="clip"> AudioClip to play </param>
				/// <param name="timeOffset"> Initial time offset for the playback </param>
				/// <param name="looping"> True, if the playback should be looping </param>
				/// <param name="settings"> Initial source settings </param>
				SourcePlayback3D(OpenALClip* clip, float timeOffset, bool looping, const AudioSource3D::Settings& settings);

			protected:
				/// <summary>
				/// Creates a new ClipPlayback instance
				/// </summary>
				/// <param name="settings"> Source settings </param>
				/// <param name="context"> Listener </param>
				/// <param name="looping"> True, if the playback is currently looping </param>
				/// <param name="timeOffset"> Initial time offset for the clip playback </param>
				/// <returns> New instance of the clip playback </returns>
				virtual Reference<ClipPlayback3D> BeginClipPlayback(const AudioSource3D::Settings& settings, ListenerContext* context, bool looping, float timeOffset) override;
			};
		}
	}
}

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
			class OpenALSource : public virtual AudioSource {
			public:
				OpenALSource(OpenALScene* scene, OpenALClip* clip);

				virtual ~OpenALSource();

				virtual int Priority()const override;

				virtual void SetPriority(int priority) override;

				virtual PlaybackState State()const override;

				virtual void Play() override;

				virtual void Pause() override;

				virtual void Stop() override;

				virtual float Time()const override;

				virtual void SetTime(float time) override;

				virtual bool Looping()const override;

				virtual void SetLooping(bool loop) override;

				virtual AudioClip* Clip()const override;

				virtual void SetClip(AudioClip* clip, bool resetTime = false) override;

			protected:
				virtual Reference<SourcePlayback> BeginPlayback(OpenALClip* clip, float timeOffset, bool looping) = 0;

				void SetPitch(float pitch);

				Reference<SourcePlayback> Playback()const;

				std::mutex& Lock()const;

			private:
				const Reference<OpenALScene> m_scene;

				mutable std::mutex m_lock;

				std::atomic<int> m_priority = 0;

				std::atomic<bool> m_looping = false;

				std::optional<std::atomic<float>> m_time;

				std::atomic<float> m_pitch = 1.0f;

				Reference<OpenALClip> m_clip;

				Reference<SourcePlayback> m_playback;

				void OnTick(float deltaTime, ActionQueue<>& queue);
			};

#pragma warning(disable: 4250)

			class OpenALSource2D : public virtual AudioSource2D, public virtual OpenALSource {
			public:
				OpenALSource2D(OpenALScene* scene, OpenALClip* clip, const AudioSource2D::Settings& settings);

				virtual void Update(const Settings& newSettings) override;

			protected:
				virtual Reference<SourcePlayback> BeginPlayback(OpenALClip* clip, float timeOffset, bool looping) override;

			private:
				AudioSource2D::Settings m_settings;
			};

			class OpenALSource3D : public virtual AudioSource3D, public virtual OpenALSource {
			public:
				OpenALSource3D(OpenALScene* scene, OpenALClip* clip, const AudioSource3D::Settings& settings);

				virtual void Update(const Settings& newSettings) override;

			protected:
				virtual Reference<SourcePlayback> BeginPlayback(OpenALClip* clip, float timeOffset, bool looping) override;

			private:
				AudioSource3D::Settings m_settings;
			};

#pragma warning(default: 4250)


			class SourcePlayback : public virtual Object {
			public:
				SourcePlayback(OpenALClip* clip, float timeOffset, bool looping);

				OpenALClip* Clip()const;

				virtual void AddListener(ListenerContext* context) = 0;

				virtual void RemoveListener(ListenerContext* context) = 0;

				virtual void RemoveAllListeners() = 0;

				virtual bool Playing();

				bool Looping()const;

				virtual void SetLooping(bool looping);

				float Time()const;

			private:
				const Reference<OpenALClip> m_clip;
				std::atomic<float> m_time;
				std::atomic<bool> m_looping;

				void AdvanceTime(float deltaTime);
				friend class OpenALSource;
			};

			template<typename SourceSettings, typename ClipPlaybackType>
			class SourcePlaybackWithClipPlaybacks : public SourcePlayback {
			private:
				typedef std::map<Reference<ListenerContext>, Reference<ClipPlaybackType>> Playbacks;
				std::mutex m_listenerLock;
				Playbacks m_playbacks;
				SourceSettings m_settings;
				std::atomic<float> m_pitch;


			protected:
				inline float Pitch()const { return m_pitch; }

				virtual Reference<ClipPlaybackType> BeginClipPlayback(const SourceSettings& settings, ListenerContext* context, bool looping, float timeOffset) = 0;


			public:
				inline SourcePlaybackWithClipPlaybacks(OpenALClip* clip, float timeOffset, bool looping, const SourceSettings& settings) 
					: SourcePlayback(clip, timeOffset, looping), m_settings(settings), m_pitch(settings.pitch) {}

				inline void Update(const SourceSettings& settings) {
					std::unique_lock<std::mutex> lock(m_listenerLock);
					m_settings = settings;
					m_pitch = settings.pitch;
					for (typename Playbacks::const_iterator it = m_playbacks.begin(); it != m_playbacks.end(); ++it)
						it->second->Update(settings);
				}

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

				inline virtual void RemoveListener(ListenerContext* context) override {
					if (context == nullptr) return;
					std::unique_lock<std::mutex> lock(m_listenerLock);
					m_playbacks.erase(context);
				}

				inline virtual void RemoveAllListeners() override {
					std::unique_lock<std::mutex> lock(m_listenerLock);
					m_playbacks.clear();
				}

				virtual bool Playing()override {
					if (SourcePlayback::Playing()) return true;
					std::unique_lock<std::mutex> lock(m_listenerLock);
					for (typename Playbacks::const_iterator it = m_playbacks.begin(); it != m_playbacks.end(); ++it)
						if (it->second->Playing()) return true;
					return false;
				}

				virtual void SetLooping(bool looping)override {
					std::unique_lock<std::mutex> lock(m_listenerLock);
					SourcePlayback::SetLooping(looping);
					for (typename Playbacks::const_iterator it = m_playbacks.begin(); it != m_playbacks.end(); ++it)
						it->second->Loop(looping);
				}
			};

			class SourcePlayback2D : public SourcePlaybackWithClipPlaybacks<AudioSource2D::Settings, ClipPlayback2D> {
			public:
				SourcePlayback2D(OpenALClip* clip, float timeOffset, bool looping, const AudioSource2D::Settings& settings);

			protected:
				virtual Reference<ClipPlayback2D> BeginClipPlayback(const AudioSource2D::Settings& settings, ListenerContext* context, bool looping, float timeOffset) override;
			};

			class SourcePlayback3D : public SourcePlaybackWithClipPlaybacks<AudioSource3D::Settings, ClipPlayback3D> {
			public:
				SourcePlayback3D(OpenALClip* clip, float timeOffset, bool looping, const AudioSource3D::Settings& settings);

			protected:
				virtual Reference<ClipPlayback3D> BeginClipPlayback(const AudioSource3D::Settings& settings, ListenerContext* context, bool looping, float timeOffset) override;
			};
		}
	}
}


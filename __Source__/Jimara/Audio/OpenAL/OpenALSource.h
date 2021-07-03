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
#include <map>


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALSource : public virtual AudioSource {
			public:

			};

			class SourcePlayback : public virtual Object {
			public:
				SourcePlayback(OpenALClip* clip, float timeOffset);

				OpenALClip* Clip()const;

				virtual void AddListener(ListenerContext* context) = 0;

				virtual void RemoveListener(ListenerContext* context) = 0;

				virtual void RemoveAllListeners() = 0;

				virtual bool Playing();

				bool Looping()const;

				float Time()const;

			private:
				const Reference<OpenALClip> m_clip;
				std::atomic<float> m_time;
			};

			template<typename SourceSettings, typename ClipPlaybackType>
			class SourcePlaybackWithClipPlaybacks : public virtual SourcePlayback {
			private:
				typedef std::map<Reference<ListenerContext>, Reference<ClipPlaybackType>> Playbacks;
				std::mutex m_listenerLock;
				Playbacks m_playbacks;
				SourceSettings m_settings;


			protected:
				virtual Reference<ClipPlaybackType> BeginClipPlayback(const SourceSettings& settings, ListenerContext* context, bool looping, float timeOffset) = 0;


			public:
				inline void Update(const SourceSettings& settings) {
					std::unique_lock<std::mutex> lock(m_listenerLock);
					m_settings = settings;
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
			};

			class SourcePlayback2D : public virtual SourcePlaybackWithClipPlaybacks<AudioSource2D::Settings, ClipPlayback2D> {
			protected:
				virtual Reference<ClipPlayback2D> BeginClipPlayback(const AudioSource2D::Settings& settings, ListenerContext* context, bool looping, float timeOffset) override;
			};

			class SourcePlayback3D : public virtual SourcePlaybackWithClipPlaybacks<AudioSource3D::Settings, ClipPlayback3D> {
			protected:
				virtual Reference<ClipPlayback3D> BeginClipPlayback(const AudioSource3D::Settings& settings, ListenerContext* context, bool looping, float timeOffset) override;
			};
		}
	}
}


#pragma once
namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALScene;
		}
	}
}
#include "OpenALClip.h"
#include "OpenALSource.h"
#include "../../Math/Helpers.h"
#include <set>
#include <unordered_set>
#include <unordered_map>


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALScene : public virtual AudioScene {
			public:
				OpenALScene(OpenALDevice* device);

				virtual ~OpenALScene();

				virtual Reference<AudioSource2D> CreateSource2D(const AudioSource2D::Settings& settings, AudioClip* clip) override;

				virtual Reference<AudioSource3D> CreateSource3D(const AudioSource3D::Settings& settings, AudioClip* clip) override;

				virtual Reference<AudioListener> CreateListener(const AudioListener::Settings& settings) override;

				void AddPlayback(SourcePlayback* playback, int priority);

				void RemovePlayback(SourcePlayback* playback);

				void AddListenerContext(ListenerContext* context);

				void RemoveListenerContext(ListenerContext* context);

			private:
				template<typename Comparator>
				struct PlaybackCompare {
					inline bool operator()(const std::pair<SourcePlayback*, int>& a, const std::pair<SourcePlayback*, int>& b)const {
						Comparator cmp;
						int pA = a.second;
						int pB = b.second;
						if (cmp(pA, pB)) return true;
						else if (cmp(pB, pA)) return false;
						else return (a.first < b.first);
					}
				};
				typedef std::unordered_map<Reference<SourcePlayback>, int> AllPlaybacks;
				typedef std::set<std::pair<SourcePlayback*, int>, PlaybackCompare<std::less<int>>> ActivePlaybacks;
				typedef std::set<std::pair<SourcePlayback*, int>, PlaybackCompare<std::greater<int>>> MutedPlaybacks;
				
				typedef std::unordered_set<Reference<ListenerContext>> AllListeners;

				std::mutex m_playbackLock;
				AllPlaybacks m_allPlaybacks;
				ActivePlaybacks m_activePlaybacks;
				MutedPlaybacks m_mutedPlaybacks;
				AllListeners m_allListeners;


				void ActivatePlayback(SourcePlayback* playback);

				void DeactivatePlayback(SourcePlayback* playback);


				void OnTick();
			};
		}
	}
}

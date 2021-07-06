#pragma once
namespace Jimara { namespace Audio { namespace OpenAL { class OpenALScene; } } }
#include "OpenALListener.h"
#include "../../Math/Helpers.h"
#include <set>
#include <unordered_set>
#include <unordered_map>


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			/// <summary>
			/// OpenAL-backed AudioScene
			/// </summary>
			class OpenALScene : public virtual AudioScene {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> Logical device, the scene resides on </param>
				OpenALScene(OpenALDevice* device);

				/// <summary> Virtual destructor </summary>
				virtual ~OpenALScene();

				/// <summary>
				/// Creates a 2D (flat/non-posed/background audio) audio source
				/// Note: When the source goes out of scope, it will automatically be removed from the scene
				/// </summary>
				/// <param name="settings"> Source settings </param>
				/// <param name="clip"> Clip to asign (can be nullptr, if you wish to set it later) </param>
				/// <returns> A new AudioSource2D that resides on the scene </returns>
				virtual Reference<AudioSource2D> CreateSource2D(const AudioSource2D::Settings& settings, AudioClip* clip) override;

				/// <summary>
				/// Creates a 3D (posed audio) audio source
				/// Note: When the source goes out of scope, it will automatically be removed from the scene
				/// </summary>
				/// <param name="settings"> Source settings </param>
				/// <param name="clip"> Clip to asign (can be nullptr, if you wish to set it later) </param>
				/// <returns> A new AudioSource3D that resides on the scene </returns>
				virtual Reference<AudioSource3D> CreateSource3D(const AudioSource3D::Settings& settings, AudioClip* clip) override;

				/// <summary>
				/// Creates an audio listener
				/// Note: When the listener goes out of scope, it will automatically be removed from the scene
				/// </summary>
				/// <param name="settings"> Listener settings </param>
				/// <returns> A new instance of an AudioListener </returns>
				virtual Reference<AudioListener> CreateListener(const AudioListener::Settings& settings) override;

				/// <summary>
				/// Adds the source playback to the set of active playbacks or changes it's priority
				/// </summary>
				/// <param name="playback"> Playback to incorporate </param>
				/// <param name="priority"> Playback priority (same as source priority) for determining which playbacks should play </param>
				void AddPlayback(SourcePlayback* playback, int priority);

				/// <summary>
				/// Removes source playback from the set of active playbacks
				/// </summary>
				/// <param name="playback"> Playback to remove </param>
				void RemovePlayback(SourcePlayback* playback);

				/// <summary>
				/// Adds listener context to active listeners
				/// </summary>
				/// <param name="context"> Context to render sound on </param>
				void AddListenerContext(ListenerContext* context);

				/// <summary>
				/// Removes listener context from active listeners
				/// </summary>
				/// <param name="context"> Context to no longer render sound on </param>
				void RemoveListenerContext(ListenerContext* context);

			private:
				// Playback comparator (by priority)
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

				// Source playback, mapped to it's priority
				typedef std::unordered_map<Reference<SourcePlayback>, int> AllPlaybacks;

				// Ascending set of currently active playbacks
				typedef std::set<std::pair<SourcePlayback*, int>, PlaybackCompare<std::less<int>>> ActivePlaybacks;

				// Descending set of playback, that are included and ready to play, but were not of high enough priority to actually play
				typedef std::set<std::pair<SourcePlayback*, int>, PlaybackCompare<std::greater<int>>> MutedPlaybacks;
				
				// Collection of all active listeners
				typedef std::unordered_set<Reference<ListenerContext>> AllListeners;


				// Lock for playback & listener addition/removal
				std::mutex m_playbackLock;

				// Source playback, mapped to it's priority
				AllPlaybacks m_allPlaybacks;

				// Ascending set of currently active playbacks
				ActivePlaybacks m_activePlaybacks;

				// Descending set of playback, that are included and ready to play, but were not of high enough priority to actually play
				MutedPlaybacks m_mutedPlaybacks;

				// Collection of all active listeners
				AllListeners m_allListeners;


				// Makes the playback "Active" by adding all active listeners to it
				void ActivatePlayback(SourcePlayback* playback);

				// Removes all active listeners form the playback
				void DeactivatePlayback(SourcePlayback* playback);
			};
		}
	}
}

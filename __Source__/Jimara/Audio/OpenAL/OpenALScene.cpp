#include "OpenALScene.h"
#include "OpenALListener.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			OpenALScene::OpenALScene(OpenALDevice* device) : AudioScene(device) {
				dynamic_cast<OpenALInstance*>(Device()->APIInstance())->OnTick() += Callback(&OpenALScene::OnTick, this);
			}

			OpenALScene::~OpenALScene() {
				dynamic_cast<OpenALInstance*>(Device()->APIInstance())->OnTick() -= Callback(&OpenALScene::OnTick, this);
			}

			Reference<AudioSource2D> OpenALScene::CreateSource2D(const AudioSource2D::Settings& settings, AudioClip* clip) {
				Device()->APIInstance()->Log()->Error("OpenALScene::CreateSource2D - Not yet implemented!");
				return nullptr;
			}

			Reference<AudioSource3D> OpenALScene::CreateSource3D(const AudioSource3D::Settings& settings, AudioClip* clip) {
				Device()->APIInstance()->Log()->Error("OpenALScene::CreateSource3D - Not yet implemented!");
				return nullptr;
			}

			Reference<AudioListener> OpenALScene::CreateListener(const AudioListener::Settings& settings) {
				return Object::Instantiate<OpenALListener>(settings, this);
			}


			void OpenALScene::AddPlayback(SourcePlayback* playback, int priority) {
				if (playback == nullptr) return;
				std::unique_lock<std::mutex> lock(m_playbackLock);
				AllPlaybacks::iterator allIt = m_allPlaybacks.find(playback);
				auto insertAsNew = [&]() {
					if (m_activePlaybacks.size() < dynamic_cast<OpenALDevice*>(Device())->MaxSources()) {
						m_activePlaybacks.insert(std::make_pair(playback, priority));
						ActivatePlayback(playback);
					}
					else {
						ActivePlaybacks::iterator it = m_activePlaybacks.begin();
						if (it->second < priority) {
							m_mutedPlaybacks.insert(*it);
							DeactivatePlayback(it->first);
							m_activePlaybacks.erase(it);
							ActivatePlayback(playback);
						}
						else m_mutedPlaybacks.insert(std::make_pair(playback, priority));
					}
				};
				if (allIt != m_allPlaybacks.end()) {
					// Change priority:
					int oldPriotity = allIt->second;
					allIt->second = priority;
					ActivePlaybacks::iterator activeIt = m_activePlaybacks.find(std::make_pair(playback, oldPriotity));
					if (activeIt != m_activePlaybacks.end()) {
						m_activePlaybacks.erase(*activeIt);
						if (!m_mutedPlaybacks.empty()) {
							MutedPlaybacks::iterator mutedIt = m_mutedPlaybacks.begin();
							if (mutedIt->second > priority) {
								DeactivatePlayback(playback);
								m_activePlaybacks.insert(*mutedIt);
								ActivatePlayback(mutedIt->first);
								m_mutedPlaybacks.erase(*mutedIt);
							}
							else m_activePlaybacks.insert(std::make_pair(playback, priority));
						}
						else m_activePlaybacks.insert(std::make_pair(playback, priority));
					}
					else {
						m_mutedPlaybacks.erase(std::make_pair(playback, oldPriotity));
						insertAsNew();
					}
				}
				else {
					// Insert new:
					m_allPlaybacks[playback] = priority;
					insertAsNew();
				}
			}

			void OpenALScene::RemovePlayback(SourcePlayback* playback) {
				if (playback == nullptr) return;
				Reference<SourcePlayback> ref;
				std::unique_lock<std::mutex> lock(m_playbackLock);
				int priority;
				{
					AllPlaybacks::iterator it = m_allPlaybacks.find(ref);
					if (it == m_allPlaybacks.end()) return;
					priority = it->second;
					m_allPlaybacks.erase(it);
				}
				{
					ActivePlaybacks::iterator it = m_activePlaybacks.find(std::make_pair(playback, priority));
					if (it != m_activePlaybacks.end()) {
						DeactivatePlayback(playback);
						m_activePlaybacks.erase(it);

						if (!m_mutedPlaybacks.empty()) {
							MutedPlaybacks::iterator it = m_mutedPlaybacks.begin();
							m_activePlaybacks.insert(*it);
							ActivatePlayback(it->first);
							m_mutedPlaybacks.erase(it);
						}

						return;
					}
				}
				m_mutedPlaybacks.erase(std::make_pair(playback, priority));
			}


			void OpenALScene::ActivatePlayback(SourcePlayback* playback) {
				// __TODO__: Play on active listener contexts
			}

			void OpenALScene::DeactivatePlayback(SourcePlayback* playback) {
				// __TODO__: Stop playback on active listener contexts
			}


			void OpenALScene::OnTick() {

			}
		}
	}
}

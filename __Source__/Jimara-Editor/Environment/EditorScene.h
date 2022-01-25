#pragma once
#include <Environment/Scene/Scene.h>
namespace Jimara {
	namespace Editor {
		class EditorScene;
	}
}
#include "JimaraEditor.h"

namespace Jimara {
	namespace Editor {
		class EditorScene : public virtual Object {
		public:
			EditorScene(EditorContext* editorContext);

			virtual ~EditorScene();

			Component* RootObject()const;

			void RequestResolution(Size2 size);

			std::recursive_mutex& UpdateLock()const;

			enum class PlayState : uint8_t {
				STOPPED = 0,
				PLAYING = 1,
				PAUSED = 2
			};

			void Play();

			void Pause();

			void Stop();

			PlayState State()const;

			Event<PlayState, const EditorScene*>& OnStateChange()const;

		private:
			// Editor Context
			const Reference<EditorContext> m_editorContext;

			// Update job
			const Reference<JobSystem::Job> m_updateJob;

			// Lock for Play/Pause/Stop
			mutable std::mutex m_stateLock;

			// Scene "Clone" is saved here when playing
			Reference<Scene> m_savedScene;

			// Current play state
			std::atomic<PlayState> m_playState = PlayState::STOPPED;

			// OnStateChange event
			mutable EventInstance<PlayState, const EditorScene*> m_onStateChange;
		};
	}
}

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
		/// <summary> Let the registry know about EditorScene </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::EditorScene);

		/// <summary>
		/// A scene instance within the Editor
		/// Note: Editor can have several EditorScene-s associated with it; just the main one is exposed through the Context
		/// </summary>
		class EditorScene : public virtual Object {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="editorContext"> Editor context </param>
			EditorScene(EditorContext* editorContext);

			/// <summary> Virtual destructor </summary>
			virtual ~EditorScene();

			/// <summary> Scene root object </summary>
			Component* RootObject()const;

			/// <summary> Editor context </summary>
			EditorContext* Context()const;

			/// <summary>
			/// Requests resolution for the output (picks highest throught frame)
			/// </summary>
			/// <param name="size"> Requested size </param>
			void RequestResolution(Size2 size);

			/// <summary> Scene Update lock </summary>
			std::recursive_mutex& UpdateLock()const;

			/// <summary>
			/// EditorScene play state
			/// </summary>
			enum class PlayState : uint8_t {
				/// <summary> Scene not simulating (ei only graphics synch & loop, and upate queue run) </summary>
				STOPPED = 0,

				/// <summary> Scene simulating </summary>
				PLAYING = 1,

				/// <summary> Scene 'paused' (same as stopped, but runtime state is preserved) </summary>
				PAUSED = 2
			};

			/// <summary> Sets play state to PLAYING </summary>
			void Play();

			/// <summary> Sets play state to PAUSED </summary>
			void Pause();

			/// <summary> Sets play state to STOPPED </summary>
			void Stop();

			/// <summary> Current play state </summary>
			PlayState State()const;

			/// <summary> Invoked when Play state changes </summary>
			Event<PlayState, const EditorScene*>& OnStateChange()const;

			/// <summary> Path to the corresponding scene asset </summary>
			std::optional<OS::Path> AssetPath()const;

			/// <summary>
			/// Loads from given path
			/// Note: Auto-updates AssetPath
			/// </summary>
			/// <param name="assetPath"> Asset path to load scene from </param>
			/// <returns> True, if load does not fail </returns>
			bool Load(const OS::Path& assetPath);

			/// <summary>
			/// Reloads scene if it has a corresponding asset path
			/// Note: Will fail if AssetPath does not exist
			/// </summary>
			/// <returns> True, if load does not fail </returns>
			bool Reload();

			/// <summary>
			/// Saves scene
			/// Note: Will fail if AssetPath does not exist
			/// </summary>
			/// <returns> True, if successful </returns>
			bool Save();

			/// <summary>
			/// Saves scene to the new asset path
			/// Note: Auto-updates AssetPath
			/// </summary>
			/// <param name="assetPath"> New asset path </param>
			/// <returns> True, if successful </returns>
			bool SaveAs(const OS::Path& assetPath);

		protected:
			/// <summary> Invoked, when reference counter reaches zero </summary>
			virtual void OnOutOfScope()const override;

		private:
			// Editor Context
			const Reference<EditorContext> m_editorContext;

			// Update job
			const Reference<JobSystem::Job> m_updateJob;

			// Lock for Play/Pause/Stop
			mutable std::mutex m_stateLock;

			// Current play state
			std::atomic<PlayState> m_playState = PlayState::STOPPED;

			// OnStateChange event
			mutable EventInstance<PlayState, const EditorScene*> m_onStateChange;

			// Invoked, when the file system DB has a change
			void OnFileSystemDBChanged(FileSystemDatabase::DatabaseChangeInfo info);
		};
	}

	// TypeIdDetails for EditorScene
	template<> void TypeIdDetails::GetParentTypesOf<Editor::EditorScene>(const Callback<TypeId>& report);
	template<> void TypeIdDetails::OnRegisterType<Editor::EditorScene>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::EditorScene>();
}

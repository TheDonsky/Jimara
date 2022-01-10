#include "EditorScene.h"


namespace Jimara {
	namespace Editor {
		EditorScene::EditorScene(EditorContext* editorContext) 
			: m_editorContext(editorContext)
			, m_scene(Object::Instantiate<Scene>(
				editorContext->ApplicationContext(), 
				editorContext->ShaderBinaryLoader(), 
				editorContext->InputModule(), 
				*editorContext->LightTypes().lightTypeIds, editorContext->LightTypes().perLightDataSize,
				editorContext->DefaultLightingModel())) {

		}

		EditorScene::~EditorScene() {

		}

		Component* EditorScene::RootObject()const { return m_scene->RootObject(); }

		std::recursive_mutex& EditorScene::UpdateLock()const {
#ifdef USE_REFACTORED_SCENE
			return m_scene->Context()->UpdateLock();
#else
			return m_scene->UpdateLock(); 
#endif
		}

		void EditorScene::Play() {
			std::unique_lock<std::mutex> lock(m_stateLock);
			m_scene->Context()->Log()->Fatal("JimaraEditorScene::Play - Not implemented!");
		}

		void EditorScene::Pause() {
			std::unique_lock<std::mutex> lock(m_stateLock);
			m_scene->Context()->Log()->Fatal("JimaraEditorScene::Pause - Not implemented!");
		}

		void EditorScene::Stop() {
			std::unique_lock<std::mutex> lock(m_stateLock);
			m_scene->Context()->Log()->Fatal("JimaraEditorScene::Stop - Not implemented!");
		}

		EditorScene::PlayState EditorScene::State()const { return m_playState; }

		Event<EditorScene::PlayState, const EditorScene*>& EditorScene::OnStateChange()const { return m_onStateChange; }
	}
}

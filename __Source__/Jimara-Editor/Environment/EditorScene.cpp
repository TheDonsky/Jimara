#include "EditorScene.h"


namespace Jimara {
	namespace Editor {
		EditorScene::EditorScene(EditorContext* editorContext)
			: m_editorContext(editorContext)
			, m_scene([&]() -> Reference<Scene> {
			Scene::CreateArgs args;
			{
				args.logic.logger = editorContext->Log();
				args.logic.input = editorContext->InputModule();
				args.logic.assetDatabase = editorContext->EditorAssetDatabase();
			}
			{
				args.graphics.graphicsDevice = editorContext->GraphicsDevice();
				args.graphics.shaderLoader = editorContext->ShaderBinaryLoader();
				args.graphics.lightSettings.lightTypeIds = editorContext->LightTypes().lightTypeIds;
				args.graphics.lightSettings.perLightDataSize = editorContext->LightTypes().perLightDataSize;
			}
			{
				args.physics.physicsInstance = editorContext->PhysicsInstance();
			}
			{
				args.audio.audioDevice = editorContext->AudioDevice();
			}
			args.createMode = Scene::CreateArgs::CreateMode::ERROR_ON_MISSING_FIELDS;
			return Scene::Create(args);
				}()) {}

		EditorScene::~EditorScene() {

		}

		Component* EditorScene::RootObject()const { return m_scene->RootObject(); }

		std::recursive_mutex& EditorScene::UpdateLock()const {
			return m_scene->Context()->UpdateLock();
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

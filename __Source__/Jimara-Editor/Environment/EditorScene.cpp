#include "EditorScene.h"
#include <Environment/Scene/SceneUpdateLoop.h>


namespace Jimara {
	namespace Editor {
		namespace {
			class EditorSceneUpdateJob : public virtual JobSystem::Job {
			public:
				const Reference<EditorContext> context;
				const Reference<Scene> scene;
				Reference<SceneUpdateLoop> updateLoop;
				Size2 requestedSize = Size2(1, 1);

				inline EditorSceneUpdateJob(EditorContext* editorContext)
					: scene([&]() -> Reference<Scene> {
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
						}()) {
					updateLoop = Object::Instantiate<SceneUpdateLoop>(scene, true);
				}

				inline virtual void Execute() final override {
					const Size2 targetSize = requestedSize;
					requestedSize = Size2(0, 0);
					if ([&]() -> bool {
						const Reference<Graphics::TextureView> texture = scene->Context()->Graphics()->Renderers().TargetTexture();
							if (texture == nullptr) return false;
							const Size3 textureSize = texture->TargetTexture()->Size();
							return (textureSize.x == targetSize.x && textureSize.y == targetSize.y);
						}()) return;
					if (targetSize.x <= 0 || targetSize.y <= 0) {
						scene->Context()->Graphics()->Renderers().SetTargetTexture(nullptr);
						return;
					}
					const Reference<Graphics::Texture> texture = scene->Context()->Graphics()->Device()->CreateMultisampledTexture(
						Graphics::Texture::TextureType::TEXTURE_2D,
						Graphics::Texture::PixelFormat::B8G8R8A8_SRGB,
						Size3(targetSize.x, targetSize.y, 1),
						1,
						Graphics::Texture::Multisampling::SAMPLE_COUNT_1);
					if (texture == nullptr) {
						context->Log()->Error("EditorScene - Failed to create target texture!");
						return;
					}
					const Reference<Graphics::TextureView> textureView = texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
					if (textureView == nullptr) {
						context->Log()->Error("EditorScene - Failed to create target texture view!");
						return;
					}
					scene->Context()->Graphics()->Renderers().SetTargetTexture(textureView);
				}
				inline virtual void CollectDependencies(Callback<Job*>) final override {}
			};
		}

		EditorScene::EditorScene(EditorContext* editorContext)
			: m_editorContext(editorContext)
			, m_updateJob(Object::Instantiate<EditorSceneUpdateJob>(editorContext)) {
			m_editorContext->AddRenderJob(m_updateJob);
		}

		EditorScene::~EditorScene() {
			m_editorContext->RemoveRenderJob(m_updateJob);
		}

		Component* EditorScene::RootObject()const { return dynamic_cast<EditorSceneUpdateJob*>(m_updateJob.operator->())->scene->RootObject(); }

		void EditorScene::RequestResolution(Size2 size) {
			EditorSceneUpdateJob* job = dynamic_cast<EditorSceneUpdateJob*>(m_updateJob.operator->());
			if (job->requestedSize.x < size.x) job->requestedSize.x = size.x;
			if (job->requestedSize.y < size.y) job->requestedSize.y = size.y;
		}

		std::recursive_mutex& EditorScene::UpdateLock()const {
			return dynamic_cast<EditorSceneUpdateJob*>(m_updateJob.operator->())->scene->Context()->UpdateLock();
		}

		void EditorScene::Play() {
			std::unique_lock<std::mutex> lock(m_stateLock);
			m_editorContext->Log()->Fatal("JimaraEditorScene::Play - Not implemented!");
		}

		void EditorScene::Pause() {
			std::unique_lock<std::mutex> lock(m_stateLock);
			m_editorContext->Log()->Fatal("JimaraEditorScene::Pause - Not implemented!");
		}

		void EditorScene::Stop() {
			std::unique_lock<std::mutex> lock(m_stateLock);
			m_editorContext->Log()->Fatal("JimaraEditorScene::Stop - Not implemented!");
		}

		EditorScene::PlayState EditorScene::State()const { return m_playState; }

		Event<EditorScene::PlayState, const EditorScene*>& EditorScene::OnStateChange()const { return m_onStateChange; }
	}
}

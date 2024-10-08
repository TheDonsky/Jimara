#include "GizmoScene.h"
#include "GizmoCreator.h"


namespace Jimara {
	namespace Editor {
		namespace {
			class GizmoContextRegistry {
			private:
				SpinLock lock;
				std::unordered_map<Reference<Scene::LogicContext>, Reference<GizmoScene::Context>> entries;

			public:
				inline static GizmoContextRegistry& Instance() {
					static GizmoContextRegistry instance;
					return instance;
				}

				inline static void Register(Scene::LogicContext* gizmoScene, GizmoScene::Context* gizmoContext) {
					if (gizmoScene == nullptr || gizmoContext == nullptr) return;
					GizmoContextRegistry& registry = Instance();
					std::unique_lock<SpinLock> lock(registry.lock);
					if (registry.entries.find(gizmoScene) != registry.entries.end())
						gizmoScene->Log()->Error("GizmoScene::GizmoContextRegistry::Register - Internal error: Entry already present! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					registry.entries[gizmoScene] = gizmoContext;
				}

				inline static void Unregister(Scene::LogicContext* gizmoScene) {
					GizmoContextRegistry& registry = Instance();
					std::unique_lock<SpinLock> lock(registry.lock);
					registry.entries.erase(gizmoScene);
				}

				inline static Reference<GizmoScene::Context> FindFor(Scene::LogicContext* gizmoScene) {
					GizmoContextRegistry& registry = Instance();
					std::unique_lock<SpinLock> lock(registry.lock);
					decltype(registry.entries)::const_iterator it = registry.entries.find(gizmoScene);
					if (it == registry.entries.end()) return nullptr;
					else {
						Reference<GizmoScene::Context> rv = it->second;
						return rv;
					}
				}
			};
		}

		Reference<GizmoScene> GizmoScene::Context::GetOwner()const {
			std::unique_lock<SpinLock> lock(m_ownerLock);
			Reference<GizmoScene> rv = m_owner;
			return rv;
		}

		Reference<GizmoScene::Context> GizmoScene::GetContext(Scene::LogicContext* gizmoContext) { return GizmoContextRegistry::FindFor(gizmoContext); }

		namespace {
			inline static Reference<Scene::LogicContext> TargetContext(EditorScene* editorScene) {
				std::unique_lock<std::recursive_mutex> lock(editorScene->UpdateLock());
				Reference<Scene::LogicContext> rv = editorScene->RootObject()->Context();
				return rv;
			}

			inline static Reference<Scene> CreateScene(EditorScene* editorScene, OS::Input* inputModule) {
				const Reference<Scene::LogicContext> targetContext = TargetContext(editorScene);

				Scene::CreateArgs createArgs;
				{
					createArgs.logic.logger = targetContext->Log();
					createArgs.logic.input = inputModule;
					createArgs.logic.assetDatabase = targetContext->AssetDB();
				}
				{
					createArgs.graphics.graphicsDevice = targetContext->Graphics()->Device();
					createArgs.graphics.shaderLibrary = targetContext->Graphics()->Configuration().ShaderLibrary();
					createArgs.graphics.maxInFlightCommandBuffers = targetContext->Graphics()->Configuration().MaxInFlightCommandBufferCount();
					createArgs.graphics.bindlessResources.bindlessArrays = targetContext->Graphics()->Bindless().Buffers();
					createArgs.graphics.bindlessResources.bindlessArrayBindings = targetContext->Graphics()->Bindless().BufferBinding();
					createArgs.graphics.bindlessResources.bindlessSamplers = targetContext->Graphics()->Bindless().Samplers();
					createArgs.graphics.bindlessResources.bindlessSamplerBindings = targetContext->Graphics()->Bindless().SamplerBinding();
				}
				{
					createArgs.physics.physicsInstance = targetContext->Physics()->APIInstance();
					createArgs.physics.sceneFlags = Physics::PhysicsInstance::SceneCreateFlags::NONE;
				}
				{
					createArgs.audio.audioDevice = targetContext->Audio()->AudioScene()->Device();
				}
				createArgs.createMode = Scene::CreateArgs::CreateMode::ERROR_ON_MISSING_FIELDS;
				Reference<Scene> scene = Scene::Create(createArgs);
				if (scene == nullptr)
					editorScene->Context()->Log()->Error("GizmoScene::Create::CreateScene - Failed to create the scene! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return scene;
			}
		}

		Reference<GizmoScene> GizmoScene::Create(EditorScene* editorScene) {
			if (editorScene == nullptr) return nullptr;
			Reference<EditorInput> inputModule = editorScene->Context()->CreateInputModule();
			Reference<Scene> scene = CreateScene(editorScene, inputModule);
			Reference<Scene::LogicContext> targetContext = TargetContext(editorScene);
			std::unique_lock<std::recursive_mutex> targetContextLock(targetContext->UpdateLock());
			while (targetContext->Graphics()->InFlightCommandBufferIndex() != scene->Context()->Graphics()->InFlightCommandBufferIndex())
				scene->SynchAndRender(editorScene->RootObject()->Context()->Time()->UnscaledDeltaTime());
			if (scene == nullptr) return nullptr;
			Reference<GizmoGUI> gizmoGUI = Object::Instantiate<GizmoGUI>(scene->Context());
			Reference<GizmoScene> result = new GizmoScene(editorScene, scene, inputModule, gizmoGUI);
			result->ReleaseRef();
			return result;
		}

		GizmoScene::GizmoScene(EditorScene* editorScene, Scene* gizmoScene, EditorInput* input, GizmoGUI* gizmoGUI)
			: m_editorScene(editorScene), m_gizmoScene(gizmoScene)
			, m_context([&]() -> Reference<Context> {
			const Reference<Scene::LogicContext> targetContext = TargetContext(m_editorScene);
			Reference<Context> context = new Context(
				targetContext, gizmoScene->Context(),
				editorScene->Selection(), editorScene->Clipboard(),
				m_editorScene->Context(), this);
			context->ReleaseRef();
			return context;
				}())
			, m_editorInput(input)
			, m_gizmoGUI(gizmoGUI) {
			GizmoContextRegistry::Register(m_gizmoScene->Context(), m_context);
			const Reference<Scene::LogicContext> targetContext = TargetContext(m_editorScene);
			targetContext->Graphics()->OnGraphicsSynch() += Callback(&GizmoScene::Update, this);
			m_gizmoCreator = Object::Instantiate<GizmoCreator>(m_context);
		}

		GizmoScene::~GizmoScene() {
			Reference<EditorScene> targetScene = m_editorScene;
			std::unique_lock<std::recursive_mutex> targetContextLock(targetScene->UpdateLock());
			m_gizmoCreator = nullptr;
			{
				std::unique_lock<SpinLock> contextLock(m_context->m_ownerLock);
				m_context->m_owner = nullptr;
			}
			m_context->TargetContext()->Graphics()->OnGraphicsSynch() -= Callback(&GizmoScene::Update, this);
			GizmoContextRegistry::Unregister(m_gizmoScene->Context());
		}

		void GizmoScene::DrawGizmoGUI()const {
			std::unique_lock<std::recursive_mutex> targetContextLock(m_context->TargetContext()->UpdateLock());
			m_gizmoGUI->Draw();
		}

		void GizmoScene::Update() {
			const size_t gizmoBufferIndex = (m_gizmoScene->Context()->Graphics()->InFlightCommandBufferIndex() + 1u) %
				m_gizmoScene->Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount();
			const size_t targetBufferIndex = m_editorScene->RootObject()->Context()->Graphics()->InFlightCommandBufferIndex();
			if (gizmoBufferIndex != targetBufferIndex)
				m_gizmoScene->Context()->Log()->Error("GizmoScene::Update - In flight buffer index mismatch ",
					"(", gizmoBufferIndex, " != ", targetBufferIndex, ")! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			m_gizmoScene->Update(m_editorScene->RootObject()->Context()->Time()->UnscaledDeltaTime());
		}
	}
}

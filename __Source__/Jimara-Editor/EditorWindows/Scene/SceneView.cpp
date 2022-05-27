#include "SceneView.h"
#include "../../GUI/Utils/DrawTooltip.h"
#include "../../Environment/EditorStorage.h"
#include "../../Gizmos/Gizmo.h"
#include "../../Gizmos/GizmoViewportHover.h"


namespace Jimara {
	namespace Editor {
		namespace {
			inline static bool UpdateGizmoScene(EditorScene* editorScene, Reference<Scene::LogicContext>& viewContext, Reference<GizmoScene>& gizmoScene) {
				Reference<Scene::LogicContext> context = editorScene->RootObject()->Context();
				if (viewContext != context) {
					viewContext = context;
					gizmoScene = GizmoScene::Create(editorScene);
					if (gizmoScene == nullptr) {
						context->Log()->Error("SceneView::UpdateGizmoScene - Failed to create GizmoScene! [File:", __FILE__, "'; Line: ", __LINE__, "]");
						return false;
					}
				}
				return true;
			}

			inline static Rect GetViewportRect() {
				auto toVec2 = [](const ImVec2& v) { return Jimara::Vector2(v.x, v.y); };
				const ImGuiStyle& style = ImGui::GetStyle();
				const Vector2 viewportOffset = toVec2(ImGui::GetItemRectSize()) * Vector2(0.0f, 1.0f) + Vector2(style.WindowBorderSize, 0.0f);;
				const Vector2 viewportPosition = toVec2(ImGui::GetWindowPos()) + viewportOffset;
				const Vector2 viewportSize = toVec2(ImGui::GetWindowSize()) - viewportOffset - Vector2(style.WindowBorderSize);
				return Rect(viewportPosition, viewportPosition + viewportSize);
			}

			inline static void RenderToViewport(
				GizmoScene* scene,
				Reference<Graphics::TextureView>& lastImage, Reference<ImGuiTexture>& lastGUITexture,
				const Rect& viewportRect, Size2& lastResolution, size_t& sameResolutionCount) {
				RenderImages* images = scene->GetContext()->Viewport()->ViewportRenderStack()->Images();
				if (images != nullptr) {
					Reference<Graphics::TextureView> image = images->GetImage(RenderImages::MainColor())->Resolve();
					if (image != nullptr) {
						if (lastImage != image) {
							const auto sampler = image->CreateSampler();
							lastImage = image;
							lastGUITexture = ImGuiRenderer::Texture(sampler);
						}
						auto toImVec = [](auto vec) { return ImVec2(vec.x, vec.y); };
						ImGui::GetWindowDrawList()->AddImage(*lastGUITexture, toImVec(viewportRect.start), toImVec(viewportRect.end));
					}
					else {
						lastImage = nullptr;
						lastGUITexture = nullptr;
					}
				}
				const Size2 currentResolution = Size2((uint32_t)viewportRect.Size().x, (uint32_t)viewportRect.Size().y);
				if (lastResolution == currentResolution) {
					if (sameResolutionCount > scene->GetContext()->GizmoContext()->Graphics()->Configuration().MaxInFlightCommandBufferCount())
						scene->GetContext()->Viewport()->SetResolution(currentResolution);
					else sameResolutionCount++;
				}
				else {
					scene->GetContext()->Viewport()->SetResolution(Size2(0u));
					sameResolutionCount = 0;
				}
				lastResolution = currentResolution;
			}
		}


		SceneView::SceneView(EditorContext* context) 
			: EditorSceneController(context), EditorWindow(context, "Scene View"), m_input(context->CreateInputModule()) {}

		SceneView::~SceneView() {
			m_gizmoScene = nullptr;
		}

		void SceneView::DrawEditorWindow() {
			Reference<EditorScene> editorScene = GetOrCreateScene();
			if (!UpdateGizmoScene(editorScene, m_viewContext, m_gizmoScene)) return;
			m_editorScene = editorScene;
			
			const Rect viewportRect = GetViewportRect();

			RenderToViewport(m_gizmoScene, m_lastImage, m_lastSampler, viewportRect, m_lastResolution, m_sameResolutionCount);
			m_gizmoScene->DrawGizmoGUI();

			m_gizmoScene->Input()->SetEnabled(ImGui::IsWindowHovered() && (!ImGui::IsAnyItemHovered()));
			m_gizmoScene->Input()->SetMouseOffset(viewportRect.start);

			if (m_gizmoScene->Input()->Enabled()) {
				const ViewportObjectQuery::Result currentResult = GizmoViewportHover::GetFor(m_gizmoScene->GetContext()->Viewport())->TargetSceneHover();
				std::unique_lock<std::recursive_mutex> lock(editorScene->UpdateLock());
				if (currentResult.component != nullptr && (!currentResult.component->Destroyed())) {
					std::string tip = [&]() {
						std::stringstream stream;
						stream << "window:" << ((size_t)this) << "; component:" << ((size_t)currentResult.component.operator->());
						return stream.str();
					}();
					DrawTooltip(tip, currentResult.component->Name(), true);
				}
			}
		}


		namespace {
			static const EditorMainMenuCallback editorMenuCallback(
				"Scene/SceneView", "Open Scene view (Scene editor window with it's own controllable camera, gizmos and similar goodies)", 
				Callback<EditorContext*>([](EditorContext* context) {
					Object::Instantiate<SceneView>(context);
					}));
			static EditorMainMenuAction::RegistryEntry action;
		}

		namespace {
			class SceneViewSerializer : public virtual EditorStorageSerializer::Of<SceneView> {
			public:
				inline SceneViewSerializer() : Serialization::ItemSerializer("SceneView", "Scene View (Editor Window)") {}

				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, SceneView* target)const final override {
					EditorWindow::Serializer()->GetFields(recordElement, target);
				}
			};
		}
	}

	template<> void TypeIdDetails::GetParentTypesOf<Editor::SceneView>(const Callback<TypeId>& report) {
		report(TypeId::Of<Editor::EditorSceneController>());
		report(TypeId::Of<Editor::EditorWindow>());
	}
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::SceneView>(const Callback<const Object*>& report) {
		static const Editor::SceneViewSerializer instance;
		report(&instance);
	}
	template<> void TypeIdDetails::OnRegisterType<Editor::SceneView>() {
		Editor::action = &Editor::editorMenuCallback;
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::SceneView>() {
		Editor::action = nullptr;
	}
}

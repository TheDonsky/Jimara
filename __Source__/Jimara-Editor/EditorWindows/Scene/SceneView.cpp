#include "SceneView.h"
#include "../../GUI/Utils/DrawTooltip.h"
#include "../../Environment/EditorStorage.h"
#include "../../Gizmos/Gizmo.h"
#include "../../Gizmos/GizmoViewportHover.h"


namespace Jimara {
	namespace Editor {
		namespace {
			static const OS::Input::KeyCode DRAG_KEY = OS::Input::KeyCode::MOUSE_RIGHT_BUTTON;
			static const OS::Input::KeyCode ROTATE_KEY = OS::Input::KeyCode::MOUSE_MIDDLE_BUTTON;

			class ViewRootObject : public virtual Gizmo {
			private:
				Reference<GizmoViewportHover> m_hover;

				inline Vector2 MousePosition() {
					return Vector2(
						Context()->Input()->GetAxis(OS::Input::Axis::MOUSE_POSITION_X),
						Context()->Input()->GetAxis(OS::Input::Axis::MOUSE_POSITION_Y));
				}

				Vector2 m_actionMousePositionOrigin = Vector2(0.0f);
				
				struct {
					Vector3 startPosition = Vector3(0.0f);
					float speed = 0.0f;
				} m_drag;

				inline bool Drag(const ViewportObjectQuery::Result& hover, Vector2 viewportSize) {
					Transform* transform = GizmoContext()->Viewport()->ViewportTransform();
					if (Context()->Input()->KeyDown(DRAG_KEY)) {
						m_drag.startPosition = transform->WorldPosition();
						if (hover.component == nullptr)
							m_drag.speed = max(m_drag.speed, 0.1f);
						else {
							Vector3 deltaPosition = (hover.objectPosition - m_drag.startPosition);
							float distance = Math::Dot(deltaPosition, transform->Forward());
							m_drag.speed = distance * std::tan(Math::Radians(GizmoContext()->Viewport()->FieldOfView()) * 0.5f) * 2.0f;
						}
						m_actionMousePositionOrigin = MousePosition();
					}
					else if (Context()->Input()->KeyPressed(DRAG_KEY)) {
						Vector2 mousePosition = MousePosition();
						Vector2 mouseDelta = (mousePosition - m_actionMousePositionOrigin) / viewportSize.y;
						transform->SetWorldPosition(m_drag.startPosition +
							m_drag.speed * (transform->Right() * -mouseDelta.x) +
							m_drag.speed * (transform->Up() * mouseDelta.y));
					}
					else return false;
					return true;
				}

				struct {
					Vector3 target = Vector3(0.0f);
					Vector3 startAngles = Vector3(0.0f);
					Vector3 startOffset = Vector3(0.0f);
					float speed = 180.0f;
				} m_rotation;

				inline bool Rotate(const ViewportObjectQuery::Result& hover, Vector2 viewportSize) {
					Transform* transform = GizmoContext()->Viewport()->ViewportTransform();
					if (Context()->Input()->KeyDown(ROTATE_KEY)) {
						if (hover.component == nullptr) {
							m_rotation.target = transform->WorldPosition();
							m_rotation.startOffset = Vector3(0.0f);
						}
						else {
							Vector3 position = transform->WorldPosition();
							Vector3 deltaPosition = (position - hover.objectPosition);
							m_rotation.startOffset = Vector3(
								Math::Dot(deltaPosition, transform->Right()),
								Math::Dot(deltaPosition, transform->Up()),
								Math::Dot(deltaPosition, transform->Forward()));
							m_rotation.target = hover.objectPosition;
						}
						m_actionMousePositionOrigin = MousePosition();
						m_rotation.startAngles = transform->WorldEulerAngles();
					}
					else if (Context()->Input()->KeyPressed(ROTATE_KEY)) {
						Vector2 mousePosition = MousePosition();
						Vector2 mouseDelta = (mousePosition - m_actionMousePositionOrigin) / viewportSize.y;
						Vector3 eulerAngles = m_rotation.startAngles + m_rotation.speed * Vector3(mouseDelta.y, mouseDelta.x, 0.0f);
						eulerAngles.x = min(max(-89.9999f, eulerAngles.x), 89.9999f);
						transform->SetWorldEulerAngles(eulerAngles);
						transform->SetWorldPosition(
							m_rotation.target +
							transform->Right() * m_rotation.startOffset.x +
							transform->Up() * m_rotation.startOffset.y +
							transform->Forward() * m_rotation.startOffset.z);
					}
					else return false;
					return true;
				}

				struct {
					float speed = 0.125f;
				} m_zoom;

				inline bool Zoom(const ViewportObjectQuery::Result& hover) {
					Transform* transform = GizmoContext()->Viewport()->ViewportTransform();
					float input = Context()->Input()->GetAxis(OS::Input::Axis::MOUSE_SCROLL_WHEEL) * m_zoom.speed;
					if (std::abs(input) <= std::numeric_limits<float>::epsilon()) return false;
					if (hover.component == nullptr)
						transform->SetWorldPosition(transform->WorldPosition() + transform->Forward() * input);
					else {
						Vector3 position = transform->WorldPosition();
						Vector3 delta = (hover.objectPosition - position);
						transform->SetWorldPosition(position + delta * min(input, 1.0f));
					}
					return true;
				}

				inline void OnGraphicsSynch() {
					Vector2 viewportSize = GizmoContext()->Viewport()->Resolution();
					const ViewportObjectQuery::Result hover = m_hover->TargetSceneHover();
					if ((!dynamic_cast<const EditorInput*>(Context()->Input())->Enabled()) 
						|| (viewportSize.x * viewportSize.y) <= std::numeric_limits<float>::epsilon()) return;
					else if (Drag(hover, viewportSize)) return;
					else if (Rotate(hover, viewportSize)) return;
					else if (Zoom(hover)) return;
				}

			public:
				inline ViewRootObject(Scene::LogicContext* context) 
					: Component(context, "ViewRootObject") {
					m_hover = GizmoViewportHover::GetFor(GizmoContext()->Viewport());
					Context()->Graphics()->OnGraphicsSynch() += Callback(&ViewRootObject::OnGraphicsSynch, this);
				}
				inline virtual ~ViewRootObject() {}

			protected:
				inline virtual void OnComponentDestroyed()final override {
					Context()->Graphics()->OnGraphicsSynch() -= Callback(&ViewRootObject::OnGraphicsSynch, this);
					m_hover = nullptr;
				}
			};


			inline static bool UpdateGizmoScene(EditorScene* editorScene, Reference<Scene::LogicContext>& viewContext, Reference<GizmoScene>& gizmoScene) {
				Reference<Scene::LogicContext> context = editorScene->RootObject()->Context();
				if (viewContext != context) {
					viewContext = context;
					gizmoScene = GizmoScene::Create(editorScene);
					if (gizmoScene == nullptr) {
						context->Log()->Error("SceneView::UpdateGizmoScene - Failed to create GizmoScene! [File:", __FILE__, "'; Line: ", __LINE__, "]");
						return false;
					}
					else Object::Instantiate<ViewRootObject>(gizmoScene->GetContext()->GizmoContext());
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

			inline static void RenderToViewport(GizmoScene* scene, const Rect& viewportRect) {
				Reference<Graphics::TextureView> image = scene->GetContext()->GizmoContext()->Graphics()->Renderers().TargetTexture();
				if (image != nullptr)
					ImGuiRenderer::Texture(image->TargetTexture(), viewportRect);
				scene->GetContext()->Viewport()->SetResolution(Size2((uint32_t)viewportRect.Size().x, (uint32_t)viewportRect.Size().y));
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

			RenderToViewport(m_gizmoScene, viewportRect);

			bool focused = ImGui::IsWindowFocused();
			m_gizmoScene->Input()->SetEnabled(focused);
			m_gizmoScene->Input()->SetMouseOffset(viewportRect.start);

			if (focused) {
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
				"Scene/SceneView", Callback<EditorContext*>([](EditorContext* context) {
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

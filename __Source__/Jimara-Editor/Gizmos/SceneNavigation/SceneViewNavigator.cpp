#include "SourceViewNavigator.h"


namespace Jimara {
	namespace Editor {
		SceneViewNavigator::SceneViewNavigator(Scene::LogicContext* context)
			: Component(context, "SceneViewNavigator") {
			m_hover = GizmoViewportHover::GetFor(GizmoContext()->Viewport());
		}

		SceneViewNavigator::~SceneViewNavigator() {}

		void SceneViewNavigator::Update() {
			Vector2 viewportSize = GizmoContext()->Viewport()->Resolution();
			const ViewportObjectQuery::Result hover = m_hover->TargetSceneHover();
			if ((!dynamic_cast<const EditorInput*>(Context()->Input())->Enabled())
				|| (viewportSize.x * viewportSize.y) <= std::numeric_limits<float>::epsilon()) return;
			else if (Drag(hover, viewportSize)) return;
			else if (Rotate(hover, viewportSize)) return;
			else if (Zoom(hover)) return;
		}

		namespace {
			static const constexpr OS::Input::KeyCode DRAG_KEY = OS::Input::KeyCode::MOUSE_RIGHT_BUTTON;
			static const constexpr OS::Input::KeyCode ROTATE_KEY = OS::Input::KeyCode::MOUSE_MIDDLE_BUTTON;

			inline static Vector2 MousePosition(Scene::LogicContext* context) {
				return Vector2(
					context->Input()->GetAxis(OS::Input::Axis::MOUSE_POSITION_X),
					context->Input()->GetAxis(OS::Input::Axis::MOUSE_POSITION_Y));
			}
		}

		bool SceneViewNavigator::Drag(const ViewportObjectQuery::Result& hover, Vector2 viewportSize) {
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
				m_actionMousePositionOrigin = MousePosition(Context());
			}
			else if (Context()->Input()->KeyPressed(DRAG_KEY)) {
				Vector2 mousePosition = MousePosition(Context());
				Vector2 mouseDelta = (mousePosition - m_actionMousePositionOrigin) / viewportSize.y;
				transform->SetWorldPosition(m_drag.startPosition +
					m_drag.speed * (transform->Right() * -mouseDelta.x) +
					m_drag.speed * (transform->Up() * mouseDelta.y));
			}
			else return false;
			return true;
		}

		bool SceneViewNavigator::Rotate(const ViewportObjectQuery::Result& hover, Vector2 viewportSize) {
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
				m_actionMousePositionOrigin = MousePosition(Context());
				m_rotation.startAngles = transform->WorldEulerAngles();
			}
			else if (Context()->Input()->KeyPressed(ROTATE_KEY)) {
				Vector2 mousePosition = MousePosition(Context());
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

		bool SceneViewNavigator::Zoom(const ViewportObjectQuery::Result& hover) {
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

		namespace {
			static const constexpr Gizmo::ComponentConnection SceneViewNavigator_GizmoConnection = Gizmo::ComponentConnection::Targetless<SceneViewNavigator>();
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::SceneViewNavigator>() {
		Editor::Gizmo::AddConnection(Editor::SceneViewNavigator_GizmoConnection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::SceneViewNavigator>() {
		Editor::Gizmo::RemoveConnection(Editor::SceneViewNavigator_GizmoConnection);
	}
}

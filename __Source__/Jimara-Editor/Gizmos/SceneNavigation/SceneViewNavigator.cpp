#include "SceneViewNavigator.h"


namespace Jimara {
	namespace Editor {
		SceneViewNavigator::SceneViewNavigator(Scene::LogicContext* context)
			: Component(context, "SceneViewNavigator") {
			m_hover = GizmoViewportHover::GetFor(GizmoContext()->Viewport());
		}

		SceneViewNavigator::~SceneViewNavigator() {}

		void SceneViewNavigator::Update() {
			Vector2 viewportSize = GizmoContext()->Viewport()->Resolution();
			
			const ViewportObjectQuery::Result handleHover = m_hover->HandleGizmoHover();
			const ViewportObjectQuery::Result sceneHover = m_hover->TargetSceneHover();
			const ViewportObjectQuery::Result gizmoHover = m_hover->SelectionGizmoHover();
			const ViewportObjectQuery::Result& hover =
				(handleHover.component != nullptr) ? handleHover :
				(gizmoHover.component != nullptr
					&& (sceneHover.component == nullptr
						|| (Math::SqrMagnitude(sceneHover.objectPosition - GizmoContext()->Viewport()->ViewportTransform()->WorldPosition()) >
							Math::SqrMagnitude(gizmoHover.objectPosition - GizmoContext()->Viewport()->ViewportTransform()->WorldPosition()))))
				? gizmoHover : sceneHover;

			if ((!dynamic_cast<const EditorInput*>(Context()->Input())->Enabled())
				|| (viewportSize.x * viewportSize.y) <= std::numeric_limits<float>::epsilon()) return;
			else if (Drag(hover, viewportSize)) return;
			else if (Rotate(hover, viewportSize)) return;
			else if (Zoom(hover)) return;
		}

		namespace {
			static const constexpr OS::Input::KeyCode DRAG_KEY = OS::Input::KeyCode::MOUSE_RIGHT_BUTTON;
			static const constexpr OS::Input::KeyCode ROTATE_KEY = OS::Input::KeyCode::MOUSE_MIDDLE_BUTTON;
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
					m_drag.speed = (GizmoContext()->Viewport()->ProjectionMode() == Camera::ProjectionMode::PERSPECTIVE)
						? (distance * std::tan(Math::Radians(GizmoContext()->Viewport()->FieldOfView()) * 0.5f) * 2.0f)
						: GizmoContext()->Viewport()->OrthographicSize();
				}
				m_actionMousePositionOrigin = m_hover->CursorPosition();
			}
			else if (Context()->Input()->KeyPressed(DRAG_KEY)) {
				Vector2 mousePosition = m_hover->CursorPosition();
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
				m_actionMousePositionOrigin = m_hover->CursorPosition();
				m_rotation.startAngles = transform->WorldEulerAngles();
			}
			else if (Context()->Input()->KeyPressed(ROTATE_KEY)) {
				Vector2 mousePosition = m_hover->CursorPosition();
				Vector2 mouseDelta = (mousePosition - m_actionMousePositionOrigin) / viewportSize.y;
				m_rotation.startAngles += m_rotation.speed * Vector3(mouseDelta.y, mouseDelta.x, 0.0f);
				m_rotation.startAngles.x = min(max(-90.0f, m_rotation.startAngles.x), 90.0f);
				m_rotation.startAngles.z = 0.0f;
				transform->SetWorldEulerAngles(m_rotation.startAngles);
				transform->SetWorldPosition(
					m_rotation.target +
					transform->Right() * m_rotation.startOffset.x +
					transform->Up() * m_rotation.startOffset.y +
					transform->Forward() * m_rotation.startOffset.z);
				m_actionMousePositionOrigin = mousePosition;
			}
			else return false;
			return true;
		}

		bool SceneViewNavigator::Zoom(const ViewportObjectQuery::Result& hover) {
			Transform* transform = GizmoContext()->Viewport()->ViewportTransform();
			const float input = Context()->Input()->GetAxis(OS::Input::Axis::MOUSE_SCROLL_WHEEL) * m_zoom.speed;
			if (std::abs(input) <= std::numeric_limits<float>::epsilon()) return false;
			if (GizmoContext()->Viewport()->ProjectionMode() == Camera::ProjectionMode::PERSPECTIVE) {
				if (hover.component == nullptr)
					transform->SetWorldPosition(transform->WorldPosition() + transform->Forward() * input);
				else {
					const Vector3 position = transform->WorldPosition();
					const Vector3 delta = (hover.objectPosition - position);
					transform->SetWorldPosition(position + delta * min(input, 1.0f));
				}
			}
			else {
				const Vector3 position = transform->WorldPosition();
				const Vector3 delta = (hover.component == nullptr) ? Vector3(0.0f) : (hover.objectPosition - position);
				const Vector3 right = transform->Right();
				const Vector3 up = transform->Up();
				const float deltaX = Math::Dot(delta, right);
				const float deltaY = Math::Dot(delta, up);
				const float scale = Math::Max(1.0f - input, 0.0f);
				GizmoContext()->Viewport()->SetOrthographicSize(GizmoContext()->Viewport()->OrthographicSize() * scale);
				transform->SetWorldPosition(
					transform->WorldPosition() +
					(transform->Forward() * input) +
					(right * deltaX * (1.0f - scale)) +
					(up * deltaY * (1.0f - scale)));
			}
			return true;
		}
	}


	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::SceneViewNavigator>(const Callback<const Object*>& report) {
		static const Reference<const Editor::Gizmo::ComponentConnection> connection =
			Editor::Gizmo::ComponentConnection::Targetless<Editor::SceneViewNavigator>();
		report(connection);
	}
}

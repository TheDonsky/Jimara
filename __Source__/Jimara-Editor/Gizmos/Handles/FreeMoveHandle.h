#pragma once
#include "Handle.h"
#include "../GizmoScene.h"
#include "../GizmoViewportHover.h"


namespace Jimara {
	namespace Editor {
		class FreeMoveHandle : public virtual Handle, public virtual Transform {
		public:
			inline FreeMoveHandle(Component* parent, const std::string_view& name)
				: Component(parent, name), Transform(parent, name)
				, m_hover(GizmoViewportHover::GetFor(GizmoScene::GetContext(parent->Context())->Viewport())) {}

			inline virtual ~FreeMoveHandle() {}

			inline Vector3 Delta()const { return m_delta; }

		protected:
			/// <summary> Invoked, if the handle is starts being dragged </summary>
			inline virtual void HandleActivated()override { 
				const ViewportObjectQuery::Result hover = m_hover->GizmoSceneHover();
				GizmoViewport* const viewport = GizmoScene::GetContext(Context())->Viewport();
				Vector3 deltaPosition = (hover.objectPosition - viewport->ViewportTransform()->WorldPosition());
				float distance = Math::Dot(deltaPosition, viewport->ViewportTransform()->Forward());
				m_dragSpeed = distance * std::tan(Math::Radians(viewport->FieldOfView()) * 0.5f) * 2.0f;
				m_lastMousePosition = CursorPosition();
			}

			/// <summary> Invoked, if the handle is dragged </summary>
			inline virtual void UpdateHandle()override {
				GizmoViewport* const viewport = GizmoScene::GetContext(Context())->Viewport();
				const Vector2 mousePosition = CursorPosition();
				Vector2 mouseDelta = (mousePosition - m_lastMousePosition) / static_cast<float>(viewport->Resolution().y);
				m_delta =
					m_dragSpeed * (viewport->ViewportTransform()->Right() * mouseDelta.x) +
					m_dragSpeed * (viewport->ViewportTransform()->Up() * -mouseDelta.y);
				m_lastMousePosition = mousePosition;
			}

			/// <summary> Invoked when handle stops being dragged (before OnHandleDeactivated()) </summary>
			inline virtual void HandleDeactivated() {
				m_delta = Vector3(0.0f);
			}

		private:
			const Reference<GizmoViewportHover> m_hover;
			
			Vector2 m_lastMousePosition = Vector2(0.0f);
			float m_dragSpeed = 0.0f;

			Vector3 m_delta = Vector3(0.0f);

			inline Vector2 CursorPosition()const {
				return Vector2(
					Context()->Input()->GetAxis(OS::Input::Axis::MOUSE_POSITION_X),
					Context()->Input()->GetAxis(OS::Input::Axis::MOUSE_POSITION_Y));
			}
		};
	}
}

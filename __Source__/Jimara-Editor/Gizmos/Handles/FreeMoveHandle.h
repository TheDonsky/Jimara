#pragma once
#include "Handle.h"
#include "../GizmoScene.h"
#include "../GizmoViewportHover.h"


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Draggable handle, that does moves freely, relative to screen space
		/// </summary>
		class FreeMoveHandle : public virtual Handle, public virtual Transform {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parent"> Parent component </param>
			/// <param name="name"> Handle name </param>
			inline FreeMoveHandle(Component* parent, const std::string_view& name)
				: Component(parent, name), Transform(parent, name)
				, m_hover(GizmoViewportHover::GetFor(GizmoScene::GetContext(parent->Context())->Viewport())) {}

			/// <summary> Virtual destructor </summary>
			inline virtual ~FreeMoveHandle() {}

			/// <summary> Drag delta from the last update cycle </summary>
			inline Vector3 Delta()const { return m_delta; }

		protected:
			/// <summary> Invoked, if the handle is starts being dragged </summary>
			inline virtual void HandleActivated()override { 
				const ViewportObjectQuery::Result hover = m_hover->GizmoSceneHover();
				GizmoViewport* const viewport = GizmoContext()->Viewport();
				Vector3 deltaPosition = (hover.objectPosition - viewport->ViewportTransform()->WorldPosition());
				float distance = Math::Dot(deltaPosition, viewport->ViewportTransform()->Forward());
				m_dragSpeed = distance * std::tan(Math::Radians(viewport->FieldOfView()) * 0.5f) * 2.0f;
				m_lastMousePosition = m_hover->CursorPosition();
			}

			/// <summary> Invoked, if the handle is dragged </summary>
			inline virtual void UpdateHandle()override {
				GizmoViewport* const viewport = GizmoContext()->Viewport();
				const Vector2 mousePosition = m_hover->CursorPosition();
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
			// Main viewport hover query
			const Reference<GizmoViewportHover> m_hover;
			
			// Last known mouse position and established drag speed
			Vector2 m_lastMousePosition = Vector2(0.0f);
			float m_dragSpeed = 0.0f;

			// Current drag 'input'
			Vector3 m_delta = Vector3(0.0f);
		};
	}
}

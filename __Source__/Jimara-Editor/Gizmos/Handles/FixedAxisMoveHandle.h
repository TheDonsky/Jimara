#pragma once
#include "Handle.h"
#include "../GizmoScene.h"
#include "../GizmoViewportHover.h"


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Handle, that is locked to move only on a specific relative axis
		/// </summary>
		class FixedAxisMoveHandle : public virtual Handle, public virtual Transform {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parent"> Parent component </param>
			/// <param name="name"> Handle name </param>
			/// <param name="axis"> Axis in local space to move along on </param>
			inline FixedAxisMoveHandle(Component* parent, const std::string_view& name, Vector3 axis = Math::Forward())
				: Component(parent, name), Transform(parent, name)
				, m_hover(GizmoViewportHover::GetFor(GizmoScene::GetContext(parent->Context())->Viewport())) {
				SetAxis(axis);
			}

			/// <summary> Virtual destructor </summary>
			inline virtual ~FixedAxisMoveHandle() {}

			/// <summary> Axis in local space to move along on </summary>
			inline Vector3 Axis()const { return m_axis; }

			/// <summary>
			/// Sets axis in local space to move along on
			/// </summary>
			/// <param name="value"> New axis </param>
			inline void SetAxis(const Vector3& value) {
				float magnitude = Math::Magnitude(value);
				m_axis = (magnitude > std::numeric_limits<float>::epsilon()) ? (value / magnitude) : Vector3(0.0f);
			}

			/// <summary> Drag delta from the last update cycle </summary>
			inline Vector3 Delta()const { return m_delta; }

		protected:
			/// <summary> Invoked, if the handle is starts being dragged </summary>
			inline virtual void HandleActivated()override {
				m_lastMousePosition = m_hover->CursorPosition();
				m_grabPosition = m_hover->GizmoSceneHover().objectPosition;
			}

			/// <summary> Invoked, if the handle is dragged </summary>
			inline virtual void UpdateHandle()override {
				// Ignore everything, if we do not have an axis:
				if (Math::SqrMagnitude(m_axis) <= std::numeric_limits<float>::epsilon()) {
					m_delta = Vector3(0.0f);
					return;
				}

				// Viewport data:
				GizmoViewport* const viewport = GizmoContext()->Viewport();
				const Transform* const view = viewport->ViewportTransform();
				const Vector3 viewPosition = view->WorldPosition();
				const Vector3 viewForward = view->Forward();

				// Processed axis:
				const Vector3 worldSpaceAxis = Math::Normalize(LocalToWorldDirection(m_axis));
				const float axisZ = Math::Dot(worldSpaceAxis, viewForward);
				const Vector3 projectedAxis = worldSpaceAxis - viewForward * axisZ;
				const float axisXY = Math::Magnitude(projectedAxis);
				if (std::abs(axisXY) <= std::numeric_limits<float>::epsilon()) {
					m_delta = Vector3(0.0f);
					return;
				}
				const Vector3 screenAxis = projectedAxis / axisXY;

				// Grabbed position stuff:
				float multiplier =
					Math::Dot(m_grabPosition - viewPosition, viewForward) *
					std::tan(Math::Radians(viewport->FieldOfView()) * 0.5f) * 2.0f;

				// Calculate raw mouse input:
				const Vector2 mousePosition = m_hover->CursorPosition();
				const Vector2 mouseDelta = (mousePosition - m_lastMousePosition) / static_cast<float>(viewport->Resolution().y);
				const Vector3 mouseRawInput = ((view->Right() * mouseDelta.x) + (view->Up() * -mouseDelta.y)) * multiplier;
				m_lastMousePosition = mousePosition;

				// Process mouse position:
				const float mouseAmount = Math::Dot(screenAxis, mouseRawInput);
				const Vector3 mouseInput = screenAxis * mouseAmount;
				const Vector3 cursorPosition = mouseInput + m_grabPosition;
				const Vector3 cursorOffset = cursorPosition - viewPosition;
				const float cursorZ = Math::Dot(cursorOffset, viewForward);
				if (cursorZ <= std::numeric_limits<float>::epsilon()) {
					m_delta = Vector3(0.0f);
					return;
				}
				const float cursorXY = Math::Dot(cursorOffset, screenAxis);

				// Result:
				const float divider = (axisXY - ((axisZ * cursorXY) / cursorZ));
				const float amount = (std::abs(divider) > std::numeric_limits<float>::epsilon()) ? (mouseAmount / divider) : 0.0f;
				m_delta = amount * worldSpaceAxis;
				m_grabPosition += m_delta;
			}

			/// <summary> Invoked when handle stops being dragged (before OnHandleDeactivated()) </summary>
			inline virtual void HandleDeactivated() {
				m_delta = Vector3(0.0f);
			}


		private:
			// Main viewport hover query
			const Reference<GizmoViewportHover> m_hover;

			// Relative direction
			Vector3 m_axis = Math::Forward();

			// Last known mouse position and established drag speed
			Vector2 m_lastMousePosition = Vector2(0.0f);
			Vector3 m_grabPosition = Vector3(0.0f);

			// Current drag 'input'
			Vector3 m_delta = Vector3(0.0f);
		};
	}
}

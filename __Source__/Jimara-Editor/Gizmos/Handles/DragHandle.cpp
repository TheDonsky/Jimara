#include "DragHandle.h"


namespace Jimara {
	namespace Editor {
		DragHandle::DragHandle(Component* parent, const std::string_view& name, Flags flags)
			: Component(parent, name), Transform(parent, name)
			, m_hover(GizmoViewportHover::GetFor(GizmoScene::GetContext(parent->Context())->Viewport()))
			, m_flags(flags) {}

		DragHandle::~DragHandle() {}

		DragHandle::Flags DragHandle::DragFlags()const { return m_flags; }

		DragHandle::Flags& DragHandle::DragFlags() { return m_flags; }

		Vector3 DragHandle::Delta()const { return m_delta; }


		void DragHandle::HandleActivated() {
			m_lastMousePosition = m_hover->CursorPosition();
			m_grabPosition = m_hover->HandleGizmoHover().objectPosition;
		}

		namespace {
			inline static Vector3 ShadowOnAxisPerspective(const Vector3& planeOffset, const Vector3& grabOffset, const Vector3& viewForward, const Vector3& axis) {
				// Processed axis:
				const float axisZ = Math::Dot(axis, viewForward);
				const Vector3 projectedAxis = axis - viewForward * axisZ;
				const float axisXY = Math::Magnitude(projectedAxis);
				if (std::abs(axisXY) <= std::numeric_limits<float>::epsilon()) return Vector3(0.0f);
				const Vector3 screenAxis = projectedAxis / axisXY;

				// Process mouse position:
				const float mouseAmount = Math::Dot(screenAxis, planeOffset);
				const Vector3 mouseInput = screenAxis * mouseAmount;
				const Vector3 cursorOffset = mouseInput + grabOffset;
				const float cursorZ = Math::Dot(cursorOffset, viewForward);
				if (cursorZ <= std::numeric_limits<float>::epsilon()) return Vector3(0.0f);
				const float cursorXY = Math::Dot(cursorOffset, screenAxis);

				// Result:
				const float divider = (axisXY - ((axisZ * cursorXY) / cursorZ));
				const float amount = (std::abs(divider) > std::numeric_limits<float>::epsilon()) ? (mouseAmount / divider) : 0.0f;
				return amount * axis;
			}

			inline static Vector3 ShadowOnAxisOrthographic(const Vector3& planeOffset, const Vector3& viewForward, const Vector3& axis) {
				const float axisZ = Math::Dot(axis, viewForward);
				const Vector3 projectedAxis = axis - viewForward * axisZ;
				const float axisXY = Math::SqrMagnitude(projectedAxis);
				if (std::abs(axisXY) <= std::numeric_limits<float>::epsilon()) return Vector3(0.0f);
				return axis* Math::Dot(planeOffset, projectedAxis / axisXY);
			}

			inline static Vector3 ShadowOnPlane(const Vector3& planeOffset, const Vector3& viewDirection, const Vector3& planeNormal) {
				// Distance to tarvel on planeNormal:
				const float distance = Math::Dot(planeOffset, planeNormal);
				if (std::abs(distance) <= std::numeric_limits<float>::epsilon())
					return planeOffset;

				// 'Speed' of closing in with the intersection point:
				const float offsetSpeed = -Math::Dot(viewDirection, planeNormal);
				if (std::abs(offsetSpeed) <= std::numeric_limits<float>::epsilon())
					return Vector3(0.0f);

				// Intersection point:
				const float time = (distance / offsetSpeed);
				const Vector3 rawIntersection = planeOffset + time * viewDirection;
				const Vector3 intersection = rawIntersection - planeNormal * (Math::Dot(planeNormal, rawIntersection));
				return intersection;
			}
		}

		void DragHandle::UpdateHandle() {
			// Initial state:
			m_delta = Vector3(0.0f);
			const Vector2 mousePosition = m_hover->CursorPosition();
			const Vector2 lastMousePosition = m_lastMousePosition;
			if (m_lastMousePosition == mousePosition) return;
			m_lastMousePosition = mousePosition;

			// Viewport data:
			GizmoViewport* const viewport = GizmoContext()->Viewport();
			const Transform* const view = viewport->ViewportTransform();
			const Vector3 viewPosition = view->WorldPosition();
			const Vector3 viewForward = view->Forward();
			const Vector3 viewRight = view->Right();
			const Vector3 viewUp = view->Up();

			// Calculate raw mouse input:
			const Vector2 mouseDelta = (mousePosition - lastMousePosition);
			const bool isPerspective = (viewport->ProjectionMode() == Camera::ProjectionMode::PERSPECTIVE);
			const float screenHeight = Math::Max(1.0f, static_cast<float>(viewport->Resolution().y));
			const float mouseMultiplier = (isPerspective
				? (Math::Dot(m_grabPosition - viewPosition, viewForward) * std::tan(Math::Radians(viewport->FieldOfView()) * 0.5f) * 2.0f)
				: viewport->OrthographicSize()) / screenHeight;
			const Vector2 mouseFlatInput = mouseDelta * mouseMultiplier;
			const Vector3 mouseRawInput = ((viewRight * mouseFlatInput.x) + (viewUp * -mouseFlatInput.y));

			// Calculate 'aligned input' vector:
			m_delta = [&]() -> Vector3 {
				auto onAxis = [&](const Vector3& axis) ->Vector3 {
					return isPerspective
						? ShadowOnAxisPerspective(mouseRawInput, m_grabPosition - viewPosition, viewForward, axis)
						: ShadowOnAxisOrthographic(mouseRawInput, viewForward, axis);
				};
				auto shadow = [&](const Vector3& up) -> Vector3 {
					return ShadowOnPlane(mouseRawInput, isPerspective ? (m_grabPosition + mouseRawInput - viewPosition) : viewForward, up);
				};
				switch (m_flags) {
				case Flags::DRAG_NONE:
					return onAxis(Vector3(0.0f));
				case Flags::DRAG_X:
					return onAxis(Right());
				case Flags::DRAG_Y:
					return onAxis(Up());
				case Flags::DRAG_Z:
					return onAxis(Forward());
				case Flags::DRAG_XY:
					return shadow(Forward());
				case Flags::DRAG_XZ:
					return shadow(Up());
				case Flags::DRAG_YZ:
					return shadow(Right());
				default:
					return mouseRawInput;
				}
			}();
			m_grabPosition += m_delta;
		}

		/// <summary> Invoked when handle stops being dragged (before OnHandleDeactivated()) </summary>
		void DragHandle::HandleDeactivated() {
			m_delta = Vector3(0.0f);
		}
	}
}

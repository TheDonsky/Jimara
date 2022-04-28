#pragma once
#include "Handle.h"
#include "../GizmoScene.h"
#include "../GizmoViewportHover.h"


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Handle, that can be dragged in interaction
		/// </summary>
		class DragHandle : public virtual Handle, public virtual Transform {
		public:
			/// <summary>
			/// Flags for controlling which local space directions the handle can be dragged in
			/// <para/> Theese flags represent a bitmask, but all variations are named
			/// </summary>
			enum class Flags : uint8_t {
				/// <summary> Drag not enabled in any direction </summary>
				DRAG_NONE = 0,

				/// <summary> Drag enabled in local X axis (Transform::Right) </summary>
				DRAG_X = (1 << 0),

				/// <summary> Drag enabled in local Y axis (Transform::Up) </summary>
				DRAG_Y = (1 << 1),

				/// <summary> Drag enabled in local Z axis (Transform::Forward) </summary>
				DRAG_Z = (1 << 2),

				/// <summary> Drag enabled in local X and Y axis (Transform::Right and Transform::Up) </summary>
				DRAG_XY = DRAG_X | DRAG_Y,

				/// <summary> Drag enabled in local X and Z axis (Transform::Right and Transform::Forward) </summary>
				DRAG_XZ = DRAG_X | DRAG_Z,

				/// <summary> Drag enabled in local Y and Z axis (Transform::Up and Transform::Forward) </summary>
				DRAG_YZ = DRAG_Y | DRAG_Z,

				/// <summary> Enable drag in any direction (same as DRAG_ANY) </summary>
				DRAG_XYZ = DRAG_X | DRAG_Y | DRAG_Z,

				/// <summary> Enable drag in any direction (same as DRAG_XYZ) </summary>
				DRAG_ANY = DRAG_XYZ
			};

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parent"> Parent component </param>
			/// <param name="name"> Handle name </param>
			/// <param name="axis"> Axis in local space to move along on </param>
			inline DragHandle(Component* parent, const std::string_view& name, Flags flags = Flags::DRAG_ANY)
				: Component(parent, name), Transform(parent, name)
				, m_hover(GizmoViewportHover::GetFor(GizmoScene::GetContext(parent->Context())->Viewport()))
				, m_flags(flags) {}

			/// <summary> Virtual destructor </summary>
			inline virtual ~DragHandle() {}

			/// <summary> Flags for controlling which local space directions the handle can be dragged in </summary>
			inline Flags DragFlags()const { return m_flags; }

			/// <summary> Flags for controlling which local space directions the handle can be dragged in </summary>
			inline Flags& DragFlags() { return m_flags; }


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
				m_delta = Vector3(0.0f);

				// Viewport data:
				GizmoViewport* const viewport = GizmoContext()->Viewport();
				const Transform* const view = viewport->ViewportTransform();
				const Vector3 viewPosition = view->WorldPosition();
				const Vector3 viewForward = view->Forward();

				// Calculate raw mouse input:
				const Vector2 mousePosition = m_hover->CursorPosition();
				const Vector2 mouseDelta = (mousePosition - m_lastMousePosition) / static_cast<float>(viewport->Resolution().y);
				m_lastMousePosition = mousePosition;
				const float mouseMultiplier =
					Math::Dot(m_grabPosition - viewPosition, viewForward) *
					std::tan(Math::Radians(viewport->FieldOfView()) * 0.5f) * 2.0f;
				const Vector3 mouseRawInput = ((view->Right() * mouseDelta.x) + (view->Up() * -mouseDelta.y)) * mouseMultiplier;
				
				// Calculate 'aligned input' vector:
				if (m_flags == Flags::DRAG_NONE) return;
				const Vector3 retainedRawInput = [&]() -> Vector3 {
					Vector3 sum = Vector3(0.0f);
					auto hasFlag = [&](Flags flag) { return (static_cast<uint8_t>(flag) & static_cast<uint8_t>(m_flags)) != 0; };
					auto normalOn = [&](const Vector3& direction) { return direction * Math::Dot(direction, mouseRawInput); };
					if (hasFlag(Flags::DRAG_X)) sum += normalOn(Right());
					if (hasFlag(Flags::DRAG_Y)) sum += normalOn(Up());
					if (hasFlag(Flags::DRAG_Z)) sum += normalOn(Forward());
					return sum;
				}();
				const float retainedRawInputAmount = Math::Magnitude(retainedRawInput);
				if (retainedRawInputAmount <= std::numeric_limits<float>::epsilon()) return;

				// Processed axis:
				const Vector3 worldSpaceAxis = (retainedRawInput / retainedRawInputAmount);
				const float axisZ = Math::Dot(worldSpaceAxis, viewForward);
				const Vector3 projectedAxis = worldSpaceAxis - viewForward * axisZ;
				const float axisXY = Math::Magnitude(projectedAxis);
				if (std::abs(axisXY) <= std::numeric_limits<float>::epsilon()) {
					m_delta = Vector3(0.0f);
					return;
				}
				const Vector3 screenAxis = projectedAxis / axisXY;


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

			// Drag flags
			Flags m_flags;

			// Last known mouse position and virtual grabbed point
			Vector2 m_lastMousePosition = Vector2(0.0f);
			Vector3 m_grabPosition = Vector3(0.0f);

			// Current drag 'input'
			Vector3 m_delta = Vector3(0.0f);
		};
	}
}


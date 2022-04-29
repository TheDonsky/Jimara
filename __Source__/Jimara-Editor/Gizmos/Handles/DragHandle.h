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
			DragHandle(Component* parent, const std::string_view& name, Flags flags = Flags::DRAG_ANY);

			/// <summary> Virtual destructor </summary>
			virtual ~DragHandle();

			/// <summary> Flags for controlling which local space directions the handle can be dragged in </summary>
			Flags DragFlags()const;

			/// <summary> Flags for controlling which local space directions the handle can be dragged in </summary>
			Flags& DragFlags();

			/// <summary> Drag delta from the last update cycle </summary>
			Vector3 Delta()const;

		protected:
			/// <summary> Invoked, if the handle is starts being dragged </summary>
			virtual void HandleActivated()override;

			/// <summary> Invoked, if the handle is dragged </summary>
			virtual void UpdateHandle()override;

			/// <summary> Invoked when handle stops being dragged (before OnHandleDeactivated()) </summary>
			virtual void HandleDeactivated()override;

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


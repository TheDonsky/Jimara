#pragma once
#include "ShapeHandles.h"


namespace Jimara {
	namespace Editor {
#pragma warning(disable: 4250)
		/// <summary>
		/// Basic handle group with 3d rotation controls
		/// <para/> Note: This one auto-resezes itself with viewport navigation
		/// </summary>
		class TripleAxisRotationHandle : public virtual Transform, public virtual Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parent"> Parent comonent </param>
			/// <param name="name"> Gizmo group name </param>
			/// <param name="size"> Gizmo size multiplier </param>
			TripleAxisRotationHandle(Component* parent, const std::string_view& name, float size = 1.0f);

			/// <summary> Virtual destructor </summary>
			~TripleAxisRotationHandle();

			/// <summary> Tells, if the underlying handles are active or not </summary>
			bool HandleActive()const;

			/// <summary> Rotation difference form the last frame  </summary>
			inline Matrix4 RotationDelta()const { return m_deltaRotation; }

			/// <summary> Cumulative rotation, since the drag started </summary>
			inline Matrix4 Rotation()const { return m_rotation; }

			/// <summary> Axis of Rotation() </summary>
			inline Vector3 RotationAxis()const { return m_axtiveHandleUp; }

			/// <summary> Rotation() angle around RotationAxis() </summary>
			inline float RotationAngle()const { return m_angle; }

			/// <summary> Invoked when handle starts being dragged </summary>
			inline Event<TripleAxisRotationHandle*>& OnHandleActivated() { return m_onHandleActivated; }

			/// <summary> Invoked on each update cycle, while the handle is being manipulated/moved around </summary>
			inline Event<TripleAxisRotationHandle*>& OnHandleUpdated() { return m_onHandleUpdated; }

			/// <summary> Invoked when handle stops being dragged </summary>
			inline Event<TripleAxisRotationHandle*>& OnHandleDeactivated() { return m_onHandleDeactivated; }

		protected:
			/// <summary> Updates component </summary>
			virtual void Update()override;

			/// <summary> Invoked, when destroyed </summary>
			virtual void OnComponentDestroyed()override;

		private:
			// Gizmo context
			const Reference<GizmoScene::Context> m_gizmoContext;

			// Hover query
			const Reference<GizmoViewportHover> m_hover;

			// Underlying handles
			const Reference<DragHandle> m_center;
			const Reference<DragHandle> m_xHandle;
			const Reference<DragHandle> m_yHandle;
			const Reference<DragHandle> m_zHandle;

			// Size multipler
			const float m_size;

			// Internal state
			Matrix4 m_deltaRotation = Math::Identity();
			Matrix4 m_rotation = Math::Identity();
			Vector3 m_axtiveHandleUp = Math::Up();
			Vector3 m_initialDragPoint = Vector3(0.0f);
			Vector3 m_dragPoint = Vector3(0.0f);
			float m_angle = 0.0f;
			

			// Handle events
			EventInstance<TripleAxisRotationHandle*> m_onHandleActivated;
			EventInstance<TripleAxisRotationHandle*> m_onHandleUpdated;
			EventInstance<TripleAxisRotationHandle*> m_onHandleDeactivated;

			// Some helper functions reside here...
			struct Helpers;

			// Underlying handle events
			void HandleActivated(Handle* handle);
			void HandleUpdated(Handle* handle);
			void HandleDisabled(Handle*);

			// Actual update function
			void UpdateScale();
		};
#pragma warning(default: 4250)
	}
}

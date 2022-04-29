#pragma once
#include "ShapeHandles.h"

namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Basic handle group with draggable directional arrows and free move handle at the center
		/// <para/> Note: This one auto-resezes itself with viewport navigation
		/// </summary>
		class TripleAxisMoveHandle : public virtual Transform, Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parent"> Parent comonent </param>
			/// <param name="name"> Gizmo group name </param>
			/// <param name="size"> Gizmo size multiplier </param>
			inline TripleAxisMoveHandle(Component* parent, const std::string_view& name, float size = 1.0f) 
				: Component(parent, name), Transform(parent, name)
				, m_center(FreeMoveSphereHandle(this, Vector4(1.0f), "Center"))
				, m_xHandle(FixedAxisArrowHandle(this, Vector4(1.0f, 0.0f, 0.0f, 1.0f), "HandleX"))
				, m_yHandle(FixedAxisArrowHandle(this, Vector4(0.0f, 1.0f, 0.0f, 1.0f), "HandleY"))
				, m_zHandle(FixedAxisArrowHandle(this, Vector4(0.0f, 0.0f, 1.0f, 1.0f), "HandleZ"))
				, m_xyHandle(FixedPlanehandle(this, Vector4(0.0f, 0.0f, 1.0f, 1.0f), "HandleXY"))
				, m_xzHandle(FixedPlanehandle(this, Vector4(0.0f, 1.0f, 0.0f, 1.0f), "HandleXZ"))
				, m_yzHandle(FixedPlanehandle(this, Vector4(1.0f, 0.0f, 0.0f, 1.0f), "HandleYZ"))
				, m_size(size) {
				const Vector3 xAngle = Vector3(0.0f, 90.0f, 90.0f);
				const Vector3 yAngle = Vector3(-90.0f, -90.0f, 0.0f);
				const Vector3 zAngle = Vector3(0.0f, 0.0f, 0.0f);
				m_xHandle->SetLocalEulerAngles(xAngle);
				m_yHandle->SetLocalEulerAngles(yAngle);
				m_zHandle->SetLocalEulerAngles(zAngle);
				m_xyHandle->SetLocalEulerAngles(zAngle);
				m_xzHandle->SetLocalEulerAngles(yAngle);
				m_yzHandle->SetLocalEulerAngles(xAngle);
				UpdateScale();
				ForAllHandles([&](Handle* handle) {
					handle->OnHandleActivated() += Callback(&TripleAxisMoveHandle::HandleActivated, this);
					handle->OnHandleUpdated() += Callback(&TripleAxisMoveHandle::HandleUpdated, this);
					handle->OnHandleDeactivated() += Callback(&TripleAxisMoveHandle::HandleDisabled, this);
					return false;
					});
			}

			/// <summary>  Virtual destructor </summary>
			inline ~TripleAxisMoveHandle() {}

			/// <summary> Tells, if the underlying handles are active or not </summary>
			inline bool HandleActive()const {
				return ForAllHandles([](Handle* handle) { return handle->HandleActive(); });
			}

			/// <summary> Sum of all underlying giozmo deltas </summary>
			inline Vector3 Delta()const { return m_delta; }

			/// <summary> Invoked when handle starts being dragged </summary>
			inline Event<TripleAxisMoveHandle*>& OnHandleActivated() { return m_onHandleActivated; }

			/// <summary> Invoked on each update cycle, while the handle is being manipulated/moved around </summary>
			inline Event<TripleAxisMoveHandle*>& OnHandleUpdated() { return m_onHandleUpdated; }

			/// <summary> Invoked when handle stops being dragged </summary>
			inline Event<TripleAxisMoveHandle*>& OnHandleDeactivated() { return m_onHandleDeactivated; }

		protected:
			/// <summary> Updates component </summary>
			inline virtual void Update()override { UpdateScale(); }

			/// <summary> Invoked, when destroyed </summary>
			inline virtual void OnComponentDestroyed() override {
				Transform::OnComponentDestroyed();
				Scene::LogicContext::UpdatingComponent::OnComponentDestroyed();
				ForAllHandles([&](Handle* handle) {
					handle->OnHandleActivated() -= Callback(&TripleAxisMoveHandle::HandleActivated, this);
					handle->OnHandleUpdated() -= Callback(&TripleAxisMoveHandle::HandleUpdated, this);
					handle->OnHandleDeactivated() -= Callback(&TripleAxisMoveHandle::HandleDisabled, this);
					return false;
					});
			}

		private:
			// underlying gizmos
			const Reference<DragHandle> m_center;
			const Reference<DragHandle> m_xHandle;
			const Reference<DragHandle> m_yHandle;
			const Reference<DragHandle> m_zHandle;
			const Reference<DragHandle> m_xyHandle;
			const Reference<DragHandle> m_xzHandle;
			const Reference<DragHandle> m_yzHandle;
			
			// Size multipler
			const float m_size;

			// Current delta
			Vector3 m_delta = Vector3(0.0f);

			// Handle events
			EventInstance<TripleAxisMoveHandle*> m_onHandleActivated;
			EventInstance<TripleAxisMoveHandle*> m_onHandleUpdated;
			EventInstance<TripleAxisMoveHandle*> m_onHandleDeactivated;

			// Underlying handle events
			inline void HandleActivated(Handle*) { m_delta = Vector3(0.0f); m_onHandleActivated(this); }
			inline void HandleUpdated(Handle* handle) { m_delta = dynamic_cast<DragHandle*>(handle)->Delta(); m_onHandleUpdated(this); }
			inline void HandleDisabled(Handle*) { m_delta = Vector3(0.0f); m_onHandleDeactivated(this); }

			template<typename CallType>
			inline bool ForAllHandles(const CallType& call)const {
				if (call(m_center)) return true;
				else if (call(m_xHandle)) return true;
				else if (call(m_yHandle)) return true;
				else if (call(m_zHandle)) return true;
				else if (call(m_xyHandle)) return true;
				else if (call(m_xzHandle)) return true;
				else if (call(m_yzHandle)) return true;
				else return false;
			}

			// Actual update function
			inline void UpdateScale() {
				SetLocalScale(Vector3(m_size * m_center->GizmoContext()->Viewport()->GizmoSizeAt(WorldPosition())));
			}
		};
	}
}

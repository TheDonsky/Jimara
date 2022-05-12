#pragma once
#include "ShapeHandles.h"
#include <Components/GraphicsObjects/MeshRenderer.h>
#include <Data/Materials/SampleDiffuse/SampleDiffuseShader.h>
#include <Data/Generators/MeshGenerator.h>


// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!
// __TODO__: Implement this crap!!!!!!!!!!!!!!

namespace Jimara {
	namespace Editor {
		class TripleAxisScalehandle : public virtual Transform, Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parent"> Parent comonent </param>
			/// <param name="name"> Gizmo group name </param>
			/// <param name="size"> Gizmo size multiplier </param>
			inline TripleAxisScalehandle(Component* parent, const std::string_view& name, float size = 1.0f) 
				: Component(parent, name), Transform(parent, name)
				, m_center(Object::Instantiate<DragHandle>(this, "XYZ"))
				, m_xHandle(Object::Instantiate<DragHandle>(this, "X", DragHandle::Flags::DRAG_X))
				, m_yHandle(Object::Instantiate<DragHandle>(this, "Y", DragHandle::Flags::DRAG_Y))
				, m_zHandle(Object::Instantiate<DragHandle>(this, "Z", DragHandle::Flags::DRAG_Z))
				, m_xyHandle(Object::Instantiate<DragHandle>(this, "XY", DragHandle::Flags::DRAG_XY))
				, m_xzHandle(Object::Instantiate<DragHandle>(this, "XZ", DragHandle::Flags::DRAG_XZ))
				, m_yzHandle(Object::Instantiate<DragHandle>(this, "YZ", DragHandle::Flags::DRAG_YZ))
				, m_size(size) {
				UpdateScale();
				auto initialize = [&](DragHandle* handle, Vector3 color, Vector3 relativePosition) {
					auto materialInstance = SampleDiffuseShader::MaterialInstance(Context()->Graphics()->Device(), color);
					Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(handle, "Renderer", Shape());
					renderer->SetMaterialInstance(materialInstance);
					renderer->SetLayer(static_cast<GraphicsLayer>(GizmoLayers::HANDLE));
					handle->SetLocalPosition(relativePosition);
				};
				initialize(m_center, Vector3(1.0f, 1.0f, 1.0f), Vector3(0.0f));
				initialize(m_xHandle, Vector3(1.0f, 0.0f, 0.0f), Vector3(0.5f, 0.0f, 0.0f));
				initialize(m_yHandle, Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 0.5f, 0.0f));
				initialize(m_zHandle, Vector3(0.0f, 0.0f, 1.0f), Vector3(0.0f, 0.0f, 0.5f));
				ForAllHandles([&](Handle* handle) {
					handle->OnHandleActivated() += Callback(&TripleAxisScalehandle::HandleActivated, this);
					handle->OnHandleUpdated() += Callback(&TripleAxisScalehandle::HandleUpdated, this);
					handle->OnHandleDeactivated() += Callback(&TripleAxisScalehandle::HandleDisabled, this);
					return false;
					});
			}

			/// <summary>  Virtual destructor </summary>
			virtual ~TripleAxisScalehandle() {}

			/// <summary> Tells, if the underlying handles are active or not </summary>
			inline bool HandleActive()const {
				return ForAllHandles([](Handle* handle) { return handle->HandleActive(); });
			}

			/// <summary> Sum of all underlying giozmo deltas </summary>
			inline Vector3 Delta()const { return m_delta; }

			/// <summary> Invoked when handle starts being dragged </summary>
			inline Event<TripleAxisScalehandle*>& OnHandleActivated() { return m_onHandleActivated; }

			/// <summary> Invoked on each update cycle, while the handle is being manipulated/moved around </summary>
			inline Event<TripleAxisScalehandle*>& OnHandleUpdated() { return m_onHandleUpdated; }

			/// <summary> Invoked when handle stops being dragged </summary>
			inline Event<TripleAxisScalehandle*>& OnHandleDeactivated() { return m_onHandleDeactivated; }

		protected:
			/// <summary> Updates component </summary>
			inline virtual void Update()override { UpdateScale(); }

			/// <summary> Invoked, when destroyed </summary>
			inline virtual void OnComponentDestroyed() override {
				Transform::OnComponentDestroyed();
				Scene::LogicContext::UpdatingComponent::OnComponentDestroyed();
				ForAllHandles([&](Handle* handle) {
					handle->OnHandleActivated() -= Callback(&TripleAxisScalehandle::HandleActivated, this);
					handle->OnHandleUpdated() -= Callback(&TripleAxisScalehandle::HandleUpdated, this);
					handle->OnHandleDeactivated() -= Callback(&TripleAxisScalehandle::HandleDisabled, this);
					return false;
					});
			}

		private:
			// Underlying handles
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
			EventInstance<TripleAxisScalehandle*> m_onHandleActivated;
			EventInstance<TripleAxisScalehandle*> m_onHandleUpdated;
			EventInstance<TripleAxisScalehandle*> m_onHandleDeactivated;

			// Underlying handle events
			inline void HandleActivated(Handle*) { m_delta = Vector3(0.0f); m_onHandleActivated(this); }
			inline void HandleUpdated(Handle* handle) { 
				Vector3 rawDelta = dynamic_cast<DragHandle*>(handle)->Delta();
				if (handle == m_center) {
					// __TODO__: Implement this crap! (scale center as we drag it);
				}
				else {
					// __TODO__: Implement this crap! (move scale handle along the scaled axis);
				}
				m_onHandleUpdated(this); 
			}
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

			inline static TriMesh* Shape() {
				static const constexpr float size = 0.1f;
				static const Reference<TriMesh> shape = GenerateMesh::Tri::Box(-Vector3(size * 0.5f), Vector3(size * 0.5f));
				return shape;
			}
		};
	}
}

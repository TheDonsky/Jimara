#pragma once
#include "ShapeHandles.h"
#include <Components/GraphicsObjects/MeshRenderer.h>
#include <Data/Materials/SampleDiffuse/SampleDiffuseShader.h>
#include <Data/Generators/MeshGenerator.h>


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
				, m_center(ScaleHandle{ Object::Instantiate<DragHandle>(this, "XYZ"), Vector3(0.0f, 0.0f, 0.0f), nullptr })
				, m_xHandle(ScaleHandle{ 
				Object::Instantiate<DragHandle>(this, "X", DragHandle::Flags::DRAG_X), 
				Vector3(0.5f, 0.0f, 0.0f), Object::Instantiate<Transform>(this, "X_Connector") })
				, m_yHandle(ScaleHandle{ 
				Object::Instantiate<DragHandle>(this, "Y", DragHandle::Flags::DRAG_Y), 
				Vector3(0.0f, 0.5f, 0.0f), Object::Instantiate<Transform>(this, "Y_Connector") })
				, m_zHandle(ScaleHandle{ 
				Object::Instantiate<DragHandle>(this, "Z", DragHandle::Flags::DRAG_Z), 
				Vector3(0.0f, 0.0f, 0.5f), Object::Instantiate<Transform>(this, "Z_Connector") })
				, m_xyHandle(ScaleHandle{ Object::Instantiate<DragHandle>(this, "XY", DragHandle::Flags::DRAG_XY), Vector3(0.0f, 0.0f, 0.0f), nullptr })
				, m_xzHandle(ScaleHandle{ Object::Instantiate<DragHandle>(this, "XZ", DragHandle::Flags::DRAG_XZ), Vector3(0.0f, 0.0f, 0.0f), nullptr })
				, m_yzHandle(ScaleHandle{ Object::Instantiate<DragHandle>(this, "YZ", DragHandle::Flags::DRAG_YZ), Vector3(0.0f, 0.0f, 0.0f), nullptr })
				, m_hover(GizmoViewportHover::GetFor(GizmoScene::GetContext(parent->Context())->Viewport()))
				, m_size(size) {
				UpdateScale();
				auto initialize = [&](const ScaleHandle& handle, Vector3 color) {
					auto materialInstance = SampleDiffuseShader::MaterialInstance(Context()->Graphics()->Device(), color);
					if (handle.handleConnector != nullptr) {
						handle.handleConnector->SetParent(handle.handle); 
						Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(handle.handleConnector, "Renderer", Shape());
						renderer->SetMaterialInstance(materialInstance);
						renderer->SetLayer(static_cast<GraphicsLayer>(GizmoLayers::HANDLE));
					}
					{
						Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(handle.handle, "Renderer", Shape());
						renderer->SetMaterialInstance(materialInstance);
						renderer->SetLayer(static_cast<GraphicsLayer>(GizmoLayers::HANDLE));
					}
					handle.SetLocalPosition(handle.defaultPosition);
				};
				initialize(m_center, Vector3(1.0f, 1.0f, 1.0f));
				initialize(m_xHandle, Vector3(1.0f, 0.0f, 0.0f));
				initialize(m_yHandle, Vector3(0.0f, 1.0f, 0.0f));
				initialize(m_zHandle, Vector3(0.0f, 0.0f, 1.0f));
				ForAllHandles([&](const ScaleHandle& handle) {
					handle.handle->OnHandleActivated() += Callback(&TripleAxisScalehandle::HandleActivated, this);
					handle.handle->OnHandleUpdated() += Callback(&TripleAxisScalehandle::HandleUpdated, this);
					handle.handle->OnHandleDeactivated() += Callback(&TripleAxisScalehandle::HandleDisabled, this);
					return false;
					});
				HandleDisabled(nullptr);
			}

			/// <summary>  Virtual destructor </summary>
			virtual ~TripleAxisScalehandle() {}

			/// <summary> Tells, if the underlying handles are active or not </summary>
			inline bool HandleActive()const {
				return ForAllHandles([](const ScaleHandle& handle) { return handle.handle->HandleActive(); });
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
				ForAllHandles([&](const ScaleHandle& handle) {
					handle.handle->OnHandleActivated() -= Callback(&TripleAxisScalehandle::HandleActivated, this);
					handle.handle->OnHandleUpdated() -= Callback(&TripleAxisScalehandle::HandleUpdated, this);
					handle.handle->OnHandleDeactivated() -= Callback(&TripleAxisScalehandle::HandleDisabled, this);
					return false;
					});
			}

		private:
			// Underlying handles
			struct ScaleHandle {
				Reference<DragHandle> handle;
				Vector3 defaultPosition;
				Reference<Transform> handleConnector;

				inline void SetLocalPosition(Vector3 position)const {
					SetWorldPosition(handle->GetComponentInParents<Transform>(false)->LocalToWorldPosition(position));
				}

				inline void SetWorldPosition(Vector3 position)const {
					handle->SetWorldPosition(position);
					if (handleConnector != nullptr) {
						handleConnector->SetLocalPosition(-handle->LocalPosition() * 0.5f);
						Vector3 rawScale = -handleConnector->LocalPosition() / (SHAPE_SIZE * 0.5f) + 0.25f;
						handleConnector->SetLocalScale(Vector3(std::abs(rawScale.x), std::abs(rawScale.y), std::abs(rawScale.z)));
					}
				}
			};
			const ScaleHandle m_center;
			const ScaleHandle m_xHandle;
			const ScaleHandle m_yHandle;
			const ScaleHandle m_zHandle;
			const ScaleHandle m_xyHandle;
			const ScaleHandle m_xzHandle;
			const ScaleHandle m_yzHandle;

			// Hover query
			const Reference<GizmoViewportHover> m_hover;

			// Size multipler
			const float m_size;

			// Current delta
			Vector3 m_hoverOrigin = Vector3(0.0f);
			Vector3 m_delta = Vector3(1.0f);

			// Handle events
			EventInstance<TripleAxisScalehandle*> m_onHandleActivated;
			EventInstance<TripleAxisScalehandle*> m_onHandleUpdated;
			EventInstance<TripleAxisScalehandle*> m_onHandleDeactivated;

			// Underlying handle events
			inline void HandleActivated(Handle*) { 
				m_hoverOrigin = m_hover->HandleGizmoHover().objectPosition;
				m_delta = Vector3(1.0f); 
				m_onHandleActivated(this); 
			}
			inline void HandleUpdated(Handle* handle) { 
				const ScaleHandle* scaleHandle = nullptr;
				ForAllHandles([&](const ScaleHandle& ref) {
					if (ref.handle == handle) {
						scaleHandle = &ref;
						return true;
					}
					else return false;
					});
				if (scaleHandle == nullptr) {
					Context()->Log()->Error("TripleAxisScalehandle::HandleUpdated - Got input from unknown handle!");
					return;
				}

				Vector3 rawDelta = dynamic_cast<DragHandle*>(handle)->Delta();
				if (scaleHandle == &m_center) {
					const Vector3 curScale = scaleHandle->handle->LocalScale();
					const Vector3 initialDelta = m_hoverOrigin - WorldPosition();
					const Vector3 deltaScale = Vector3(Math::Dot(rawDelta, initialDelta) / Math::SqrMagnitude(initialDelta));
					const Vector3 finalScale = curScale + deltaScale;
					scaleHandle->handle->SetLocalScale(finalScale);
					m_delta = (finalScale / curScale);
				}
				else {
					// __TODO__: Implement for XY/XZ/YZ...
					const Vector3 scaleDirection = Math::Normalize(LocalToWorldDirection(scaleHandle->defaultPosition));
					auto scaleFactor = [&]() { return Math::Dot(scaleHandle->handle->WorldPosition() - WorldPosition(), scaleDirection); };
					float curVal = scaleFactor();
					scaleHandle->SetWorldPosition(
						scaleHandle->handle->WorldPosition() +
						(scaleDirection * Math::Dot(rawDelta, scaleDirection)));
					float newValue = scaleFactor();
					m_delta = (scaleDirection * ((newValue / curVal) - 1.0f)) + 1.0f;
				}
				m_onHandleUpdated(this); 
			}
			inline void HandleDisabled(Handle*) { 
				m_delta = Vector3(1.0f);
				ForAllHandles([](const ScaleHandle& handle) { 
					handle.SetLocalPosition(handle.defaultPosition);
					return false;
					});
				m_center.handle->SetLocalScale(Vector3(2.0f));
				m_onHandleDeactivated(this); 
			}

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
				SetLocalScale(Vector3(m_size * m_center.handle->GizmoContext()->Viewport()->GizmoSizeAt(WorldPosition())));
			}

			static const constexpr float SHAPE_SIZE = 0.075f;

			inline static TriMesh* Shape() {
				static const Reference<TriMesh> shape = GenerateMesh::Tri::Box(-Vector3(SHAPE_SIZE * 0.5f), Vector3(SHAPE_SIZE * 0.5f));
				return shape;
			}
		};
	}
}

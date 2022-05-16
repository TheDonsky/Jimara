#include "TripleAxisRotationHandle.h"
#include <Components/GraphicsObjects/MeshRenderer.h>
#include <Data/Materials/SampleDiffuse/SampleDiffuseShader.h>
#include <Data/Generators/MeshGenerator.h>


namespace Jimara {
	namespace Editor {
		struct TripleAxisRotationHandle::Helpers {
			inline static void InitializeCenter(DragHandle* handle) {
				auto materialInstance = SampleDiffuseShader::MaterialInstance(handle->Context()->Graphics()->Device(), Vector3(1.0f));
				static const Reference<TriMesh> shape = GenerateMesh::Tri::Sphere(Vector3(0.0f), 0.4f, 32, 16);
				Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(handle, "Renderer", shape);
				renderer->SetMaterialInstance(materialInstance);
				renderer->SetLayer(static_cast<GraphicsLayer>(GizmoLayers::HANDLE_INVISIBLE));
			}

			inline static void Initialize(DragHandle* handle, Vector3 color, Vector3 rotation) {
				auto materialInstance = SampleDiffuseShader::MaterialInstance(handle->Context()->Graphics()->Device(), color);
				{
					static const Reference<TriMesh> shape = GenerateMesh::Tri::Torus(Vector3(0.0f), 0.45f, 0.0125f, 64, 4);
					Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(handle, "Renderer", shape);
					renderer->SetMaterialInstance(materialInstance);
					renderer->SetLayer(static_cast<GraphicsLayer>(GizmoLayers::HANDLE));
				}
				{
					static const Reference<TriMesh> shape = GenerateMesh::Tri::Torus(Vector3(0.0f), 0.45f, 0.025f, 64, 4);
					Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(handle, "Renderer", shape);
					renderer->SetMaterialInstance(materialInstance);
					renderer->SetLayer(static_cast<GraphicsLayer>(GizmoLayers::HANDLE_INVISIBLE));
				}
				handle->SetLocalEulerAngles(rotation);
			};

			template<typename CallType>
			inline static bool ForAllHandles(const TripleAxisRotationHandle* self, const CallType& call) {
				if (call(self->m_center)) return true;
				else if (call(self->m_xHandle)) return true;
				else if (call(self->m_yHandle)) return true;
				else if (call(self->m_zHandle)) return true;
				else return false;
			}
		};

		TripleAxisRotationHandle::TripleAxisRotationHandle(Component* parent, const std::string_view& name, float size) 
			: Component(parent, name), Transform(parent, name)
			, m_gizmoContext(GizmoScene::GetContext(parent->Context()))
			, m_hover(GizmoViewportHover::GetFor(GizmoScene::GetContext(parent->Context())->Viewport()))
			, m_center(Object::Instantiate<DragHandle>(this, "XYZ", DragHandle::Flags::DRAG_XYZ))
			, m_xHandle(Object::Instantiate<DragHandle>(this, "X", DragHandle::Flags::DRAG_XZ))
			, m_yHandle(Object::Instantiate<DragHandle>(this, "Y", DragHandle::Flags::DRAG_XZ))
			, m_zHandle(Object::Instantiate<DragHandle>(this, "Z", DragHandle::Flags::DRAG_XZ))
			, m_size(size) {
			Helpers::InitializeCenter(m_center);
			Helpers::Initialize(m_xHandle, Vector3(1.0f, 0.0f, 0.0f), Vector3(-90.0f, -90.0f, 0.0f));
			Helpers::Initialize(m_yHandle, Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f));
			Helpers::Initialize(m_zHandle, Vector3(0.0f, 0.0f, 1.0f), Vector3(0.0f, 90.0f, 90.0f));
			Helpers::ForAllHandles(this, [&](const DragHandle* handle) {
				handle->OnHandleActivated() += Callback(&TripleAxisRotationHandle::HandleActivated, this);
				handle->OnHandleUpdated() += Callback(&TripleAxisRotationHandle::HandleUpdated, this);
				handle->OnHandleDeactivated() += Callback(&TripleAxisRotationHandle::HandleDisabled, this);
				return false;
				});
			HandleDisabled(nullptr);
			UpdateScale();
		}

		TripleAxisRotationHandle::~TripleAxisRotationHandle() {}

		bool TripleAxisRotationHandle::HandleActive()const {
			return Helpers::ForAllHandles(this, [](const DragHandle* handle) { return handle->HandleActive(); });
		}

		void TripleAxisRotationHandle::Update() { UpdateScale(); }

		void TripleAxisRotationHandle::OnComponentDestroyed() {
			Transform::OnComponentDestroyed();
			Scene::LogicContext::UpdatingComponent::OnComponentDestroyed();
			Helpers::ForAllHandles(this, [&](const DragHandle* handle) {
				handle->OnHandleActivated() -= Callback(&TripleAxisRotationHandle::HandleActivated, this);
				handle->OnHandleUpdated() -= Callback(&TripleAxisRotationHandle::HandleUpdated, this);
				handle->OnHandleDeactivated() -= Callback(&TripleAxisRotationHandle::HandleDisabled, this);
				return false;
				});
		}

		void TripleAxisRotationHandle::HandleActivated(Handle* handle) {
			m_deltaRotation = m_rotation = Math::Identity();
			m_dragPoint = m_hover->HandleGizmoHover().objectPosition - WorldPosition();
			m_onHandleActivated(this);
		}
		void TripleAxisRotationHandle::HandleUpdated(Handle* handle) {
			DragHandle* dragHandle = dynamic_cast<DragHandle*>(handle);
			const Vector3 newDragPoint = m_dragPoint + dragHandle->Delta();

			auto safeNormalize = [](const Vector3& value) { 
				const float magnitude = Math::Magnitude(value);
				return (magnitude > std::numeric_limits<float>::epsilon()) ? (value / magnitude) : Vector3(0.0f);
			};
			const Vector3& handleUp = (handle == m_center) ? safeNormalize(Math::Cross(m_dragPoint, newDragPoint)) : dragHandle->Up();
			auto projectAndNormalize = [&](const Vector3& offset) { return safeNormalize(offset - (handleUp * Math::Dot(offset, handleUp))); };
			
			const Vector3 oldDragDirection = projectAndNormalize(m_dragPoint);
			const Vector3 newDragDirection = projectAndNormalize(newDragPoint);
			
			if (std::isnan(oldDragDirection.x) || std::isnan(oldDragDirection.y) || std::isnan(oldDragDirection.z) ||
				std::isnan(newDragDirection.x) || std::isnan(newDragDirection.y) || std::isnan(newDragDirection.z)) {
				Context()->Log()->Error("TripleAxisRotationHandle::HandleUpdated - Nan-s calculated! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				m_deltaRotation = Math::Identity();
			}
			else {
				const float angleCos = Math::Min(Math::Max(-1.0f, Math::Dot(oldDragDirection, newDragDirection)), 1.0f);
				const float angleSign = (Math::Dot(handleUp, Math::Cross(oldDragDirection, newDragDirection)) > 0.0f) ? 1.0f : (-1.0f);
				const float angle = Math::Degrees(std::acos(angleCos)) * angleSign;
				m_deltaRotation = Math::ToMatrix(Math::AxisAngle(handleUp, angle));
			}
			m_rotation = (m_deltaRotation * m_rotation);
			m_dragPoint = newDragPoint;
			m_onHandleUpdated(this);
		}
		void TripleAxisRotationHandle::HandleDisabled(Handle*) {
			m_deltaRotation = m_rotation = Math::Identity();
			m_dragPoint = Vector3(0.0f);
			m_onHandleDeactivated(this);
		}

		void TripleAxisRotationHandle::UpdateScale() {
			SetLocalScale(Vector3(m_size * m_gizmoContext->Viewport()->GizmoSizeAt(WorldPosition())));
		}
	}
}

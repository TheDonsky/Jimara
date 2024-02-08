#include "PointLightGizmo.h"
#include <Jimara/Math/Random.h>
#include <Jimara/Data/Geometry/MeshGenerator.h>
#include <Jimara/Data/Geometry/MeshModifiers.h>
#include <Jimara/Components/Lights/PointLight.h>
#include <Jimara/Components/GraphicsObjects/MeshRenderer.h>
#include "../../../Handles/Compound/SphereResizeHandle.h"


namespace Jimara {
	namespace Editor {
		namespace {
			static const Reference<TriMesh> LIGHT_SHAPE = []()->Reference<TriMesh> {
				const float radius = 0.075f;
				const Reference<TriMesh> center = GenerateMesh::Tri::Sphere(Vector3(0.0f), radius, 16, 8);

				const Vector3 rayHalfSize = Vector3(0.005f, 0.005f, 0.05f);
				const Vector3 rayPosition = Math::Forward() * (radius + rayHalfSize.z + 0.05f);
				const Reference<TriMesh> ray = GenerateMesh::Tri::Box(rayPosition - rayHalfSize, rayPosition + rayHalfSize);
				
				const Reference<TriMesh> ray_tilt_45 = ModifyMesh::Transform(ray, Math::MatrixFromEulerAngles(Vector3(45.0f, 45.0f, 0.0f)));
				const Reference<TriMesh> ray_ntilt_45 = ModifyMesh::Transform(ray, Math::MatrixFromEulerAngles(Vector3(-45.0f, 45.0f, 0.0f)));
				const Reference<TriMesh> ray_tilt = ModifyMesh::Merge(ray_tilt_45, ray_ntilt_45, "ray_tilt");
				const Reference<TriMesh> rays_0 = ModifyMesh::Merge(ray, ray_tilt, "rays_0");

				const Reference<TriMesh> rays_180 = ModifyMesh::Transform(rays_0, Math::MatrixFromEulerAngles(Vector3(0.0f, 180.0f, 0.0f)));
				const Reference<TriMesh> rays_0_180 = ModifyMesh::Merge(rays_0, rays_180, "rays_0_180");

				const Reference<TriMesh> rays_90_270 = ModifyMesh::Transform(rays_0_180, Math::MatrixFromEulerAngles(Vector3(0.0f, 90.0f, 0.0f)));
				const Reference<TriMesh> rays_90s = ModifyMesh::Merge(rays_0_180, rays_90_270, "rays_90s");

				const Reference<TriMesh> ray_up = ModifyMesh::Transform(ray, Math::MatrixFromEulerAngles(Vector3(90.0f, 0.0f, 0.0f)));
				const Reference<TriMesh> ray_down = ModifyMesh::Transform(ray, Math::MatrixFromEulerAngles(Vector3(-90.0f, 0.0f, 0.0f)));
				const Reference<TriMesh> rays_u_d = ModifyMesh::Merge(ray_up, ray_down, "rays_u_d");

				const Reference<TriMesh> rays = ModifyMesh::Merge(rays_90s, rays_u_d, "rays_u_d");
				return ModifyMesh::Merge(center, rays, "PointLight");
			}();

			class PointLightResizeHandle : public virtual Gizmo, public virtual SceneContext::UpdatingComponent {
			private:
				const Reference<SphereResizeHandle> m_resizeHandle;

			public:
				inline PointLightResizeHandle(Scene::LogicContext* context) 
					: Component(context, "PointLightResizeHandle")
					, m_resizeHandle(Object::Instantiate<SphereResizeHandle>(this, Vector3(1.0f, 1.0f, 0.0f))) {
					m_resizeHandle->SetEnabled(false);
				}

				inline virtual ~PointLightResizeHandle() {}

				inline virtual void Update() override {
					Reference<PointLight> target = Target<PointLight>();
					if (target == nullptr) return;
					Transform* targetTransform = target->GetTransfrom();
					if (targetTransform != nullptr && target->ActiveInHeirarchy()) {
						float radius = target->Radius();
						m_resizeHandle->SetEnabled(true);
						m_resizeHandle->Update(targetTransform->WorldPosition(), targetTransform->WorldEulerAngles(), radius);
						target->SetRadius(radius);
					}
					else m_resizeHandle->SetEnabled(false);
				}
			};
		}

		PointLightGizmo::PointLightGizmo(Scene::LogicContext* context)
			: Component(context, "PointLightGizmo")
			, m_handle(Object::Instantiate<Transform>(this, "PointLightGizmo_Handle")) {
			Object::Instantiate<MeshRenderer>(m_handle, "PointLightGizmo_Renderer", LIGHT_SHAPE)->SetLayer(static_cast<Layer>(GizmoLayers::SELECTION_OVERLAY));
		}

		PointLightGizmo::~PointLightGizmo() {}

		void PointLightGizmo::Update() {
			Transform* gizmoTransform = dynamic_cast<Transform*>(m_handle.operator->());
			Component* target = TargetComponent();
			if (target == nullptr) return;
			Transform* targetTransform = target->GetTransfrom();
			if (targetTransform != nullptr && target->ActiveInHeirarchy()) {
				gizmoTransform->SetEnabled(true);
				gizmoTransform->SetWorldPosition(targetTransform->WorldPosition());
				gizmoTransform->SetWorldEulerAngles(targetTransform->WorldEulerAngles());
			}
			else gizmoTransform->SetEnabled(false);
		}

		namespace {
			static const constexpr Gizmo::ComponentConnection PointLightGizmo_Connection =
				Gizmo::ComponentConnection::Make<PointLightGizmo, PointLight>(
					Gizmo::FilterFlag::CREATE_IF_SELECTED |
					Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED |
					Gizmo::FilterFlag::CREATE_CHILD_GIZMOS_IF_SELECTED |
					Gizmo::FilterFlag::CREATE_PARENT_GIZMOS_IF_SELECTED);
			static const constexpr Gizmo::ComponentConnection PointLightResizeHandle_Connection =
				Gizmo::ComponentConnection::Make<PointLightResizeHandle, PointLight>();
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::PointLightGizmo>() {
		Editor::Gizmo::AddConnection(Editor::PointLightGizmo_Connection);
		Editor::Gizmo::AddConnection(Editor::PointLightResizeHandle_Connection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::PointLightGizmo>() {
		Editor::Gizmo::RemoveConnection(Editor::PointLightGizmo_Connection);
		Editor::Gizmo::RemoveConnection(Editor::PointLightResizeHandle_Connection);
	}
}

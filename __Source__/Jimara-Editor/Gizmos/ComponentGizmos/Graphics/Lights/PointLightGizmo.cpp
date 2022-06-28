#include "PointLightGizmo.h"
#include <Math/Random.h>
#include <Data/Generators/MeshGenerator.h>
#include <Data/Generators/MeshModifiers.h>
#include <Components/Lights/PointLight.h>
#include <Components/GraphicsObjects/MeshRenderer.h>


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

#pragma warning(disable: 4250)
			class PointLightGizmo_Handle : public virtual Handle, public virtual Transform {
			public:
				inline PointLightGizmo_Handle(Component* gizmo) : Component(gizmo), Transform(gizmo, "PointLightGizmo_Handle") {
					Object::Instantiate<MeshRenderer>(this, "PointLightGizmo_Renderer", LIGHT_SHAPE)->SetLayer(static_cast<GraphicsLayer>(GizmoLayers::SELECTION_OVERLAY));
				}
				inline virtual ~PointLightGizmo_Handle() {}

			protected:
				inline virtual void HandleActivated() override {
					// __TODO__: Once we add layers, there should be no handle here at all, just a selection handle
					Gizmo* gizmo = GetComponentInParents<Gizmo>();
					if (gizmo == nullptr || gizmo->TargetCount() <= 0) return;
					Component* target = gizmo->TargetComponent();
					if (Context()->Input()->KeyPressed(OS::Input::KeyCode::LEFT_CONTROL)
						|| Context()->Input()->KeyPressed(OS::Input::KeyCode::RIGHT_CONTROL))
						GizmoContext()->Selection()->Select(target);
					else if (Context()->Input()->KeyPressed(OS::Input::KeyCode::LEFT_ALT)
						|| Context()->Input()->KeyPressed(OS::Input::KeyCode::RIGHT_ALT))
						GizmoContext()->Selection()->Deselect(target);
					else {
						GizmoContext()->Selection()->DeselectAll();
						GizmoContext()->Selection()->Select(target);
					}
				}
			};
#pragma warning(default: 4250)
		}

		PointLightGizmo::PointLightGizmo(Scene::LogicContext* context)
			: Component(context, "PointLightGizmo")
			, m_handle(Object::Instantiate<PointLightGizmo_Handle>(this)) {}

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
			static const constexpr Gizmo::ComponentConnection TransformGizmo_Connection =
				Gizmo::ComponentConnection::Make<PointLightGizmo, PointLight>(
					Gizmo::FilterFlag::CREATE_IF_SELECTED |
					Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED |
					Gizmo::FilterFlag::CREATE_CHILD_GIZMOS_IF_SELECTED |
					Gizmo::FilterFlag::CREATE_PARENT_GIZMOS_IF_SELECTED);
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::PointLightGizmo>() {
		Editor::Gizmo::AddConnection(Editor::TransformGizmo_Connection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::PointLightGizmo>() {
		Editor::Gizmo::RemoveConnection(Editor::TransformGizmo_Connection);
	}
}

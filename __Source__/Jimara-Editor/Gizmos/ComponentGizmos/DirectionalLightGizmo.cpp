#include "DirectionalLightGizmo.h"
#include <Math/Random.h>
#include <Data/Generators/MeshGenerator.h>
#include <Data/Generators/MeshModifiers.h>
#include <Components/Lights/DirectionalLight.h>
#include <Components/GraphicsObjects/MeshRenderer.h>


namespace Jimara {
	namespace Editor {
		namespace {
			static const Reference<TriMesh> LIGHT_SHAPE = [&]()->Reference<TriMesh> {
				const float radius = 0.075f;
				const Reference<TriMesh> center = GenerateMesh::Tri::Sphere(Vector3(0.0f), radius, 16, 8);

				const Vector3 rayHalfSize = Vector3(0.005f, 0.005f, 0.05f);
				const Vector3 rayPosition = Math::Forward() * (radius + rayHalfSize.z + 0.05f);
				const Reference<TriMesh> ray = GenerateMesh::Tri::Box(rayPosition - rayHalfSize, rayPosition + rayHalfSize);

				const Reference<TriMesh> ray0 = [&]() {
					Matrix4 transform = Math::Identity();
					transform[3] = Vector4(-0.075, 0.0f, -0.05f, 1.0f);
					return ModifyMesh::Transform(ray, transform);
				}();

				const Reference<TriMesh> ray180 = ModifyMesh::Transform(ray0, Math::MatrixFromEulerAngles(Vector3(0.0f, 0.0f, 180.0f)));
				const Reference<TriMesh> ray0_180 = ModifyMesh::Merge(ray0, ray180, "ray0_180");
				const Reference<TriMesh> ray90_270 = ModifyMesh::Transform(ray0_180, Math::MatrixFromEulerAngles(Vector3(0.0f, 0.0f, 90.0f)), "ray90_270");
				
				const Reference<TriMesh> ray90s = ModifyMesh::Merge(ray0_180, ray90_270, "ray90s");
				const Reference<TriMesh> ray45s = ModifyMesh::Transform(ray90s, Math::MatrixFromEulerAngles(Vector3(0.0f, 0.0f, 45.0f)), "ray45s");
				
				const Reference<TriMesh> sideRays = ModifyMesh::Merge(ray90s, ray45s, "sideRays");
				const Reference<TriMesh> allRays = ModifyMesh::Merge(ray, sideRays, "allRays");
				return ModifyMesh::Merge(center, allRays, "DirectionalLight");
			}();

			class DirectionalLightGizmo_Handle : public virtual Handle, public virtual Transform {
			public:
				inline DirectionalLightGizmo_Handle(Component* gizmo) : Component(gizmo), Transform(gizmo, "DirectionalLightGizmo_Handle") {
					Object::Instantiate<MeshRenderer>(this, "DirectionalLightGizmo_Renderer", LIGHT_SHAPE);
				}
				inline virtual ~DirectionalLightGizmo_Handle() {}

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
		}

		DirectionalLightGizmo::DirectionalLightGizmo(Scene::LogicContext* context)
			: Component(context, "DirectionalLightGizmo")
			, m_handle(Object::Instantiate<DirectionalLightGizmo_Handle>(this)) {}

		DirectionalLightGizmo::~DirectionalLightGizmo() {}

		void DirectionalLightGizmo::Update() {
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
				Gizmo::ComponentConnection::Make<DirectionalLightGizmo, DirectionalLight>(
					Gizmo::FilterFlag::CREATE_IF_SELECTED |
					Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED |
					Gizmo::FilterFlag::CREATE_CHILD_GIZMOS_IF_SELECTED |
					Gizmo::FilterFlag::CREATE_PARENT_GIZMOS_IF_SELECTED);
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::DirectionalLightGizmo>() {
		Editor::Gizmo::AddConnection(Editor::TransformGizmo_Connection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::DirectionalLightGizmo>() {
		Editor::Gizmo::RemoveConnection(Editor::TransformGizmo_Connection);
	}
}

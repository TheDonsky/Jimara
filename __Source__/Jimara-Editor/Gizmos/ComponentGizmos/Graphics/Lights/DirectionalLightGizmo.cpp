#include "DirectionalLightGizmo.h"
#include <Jimara/Math/Random.h>
#include <Jimara/Data/Geometry/MeshGenerator.h>
#include <Jimara/Data/Geometry/MeshModifiers.h>
#include <Jimara/Components/Lights/DirectionalLight.h>
#include <Jimara/Components/GraphicsObjects/MeshRenderer.h>


namespace Jimara {
	namespace Editor {
		namespace {
			static const Reference<TriMesh> LIGHT_SHAPE = []()->Reference<TriMesh> {
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
		}

		DirectionalLightGizmo::DirectionalLightGizmo(Scene::LogicContext* context)
			: Component(context, "DirectionalLightGizmo")
			, m_handle(Object::Instantiate<Transform>(this, "DirectionalLightGizmo_Transform")) {
			Object::Instantiate<MeshRenderer>(m_handle, "DirectionalLightGizmo_Renderer", LIGHT_SHAPE)->SetLayer(static_cast<Layer>(GizmoLayers::SELECTION_OVERLAY));
		}

		DirectionalLightGizmo::~DirectionalLightGizmo() {}

		void DirectionalLightGizmo::Update() {
			Component* target = TargetComponent();
			if (target == nullptr) return;
			Transform* targetTransform = target->GetTransfrom();
			if (targetTransform != nullptr && target->ActiveInHierarchy()) {
				m_handle->SetEnabled(true);
				m_handle->SetWorldPosition(targetTransform->WorldPosition());
				m_handle->SetWorldEulerAngles(targetTransform->WorldEulerAngles());
			}
			else m_handle->SetEnabled(false);
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::DirectionalLightGizmo>(const Callback<const Object*>& report) {
		static const Reference<const Editor::Gizmo::ComponentConnection> connection =
			Editor::Gizmo::ComponentConnection::Make<Editor::DirectionalLightGizmo, DirectionalLight>(
				Editor::Gizmo::FilterFlag::CREATE_IF_SELECTED |
				Editor::Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED |
				Editor::Gizmo::FilterFlag::CREATE_CHILD_GIZMOS_IF_SELECTED |
				Editor::Gizmo::FilterFlag::CREATE_PARENT_GIZMOS_IF_SELECTED);
		report(connection);
	}
}

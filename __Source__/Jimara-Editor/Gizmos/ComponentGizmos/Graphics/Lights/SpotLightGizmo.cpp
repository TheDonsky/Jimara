#include "SpotLightGizmo.h"
#include <Components/Lights/SpotLight.h>
#include <Data/Generators/MeshGenerator.h>
#include <Data/Generators/MeshModifiers.h>
#include <Components/GraphicsObjects/MeshRenderer.h>


namespace Jimara {
	namespace Editor {
		struct SpotLightGizmo::Helpers {
			inline static TriMesh* LightShape() {
				static const Reference<TriMesh> LIGHT_SHAPE = []()->Reference<TriMesh> {
					const float radius = 0.125f;
					const Reference<TriMesh> center = GenerateMesh::Tri::Cylinder(Vector3(0.0f, -radius, 0.0f), radius, radius * 1.25f, 16);
					const Reference<TriMesh> rotatedCenter = ModifyMesh::Transform(center, Math::MatrixFromEulerAngles(Vector3(-90.0f, 0.0f, 0.0f)));

					const Vector3 rayHalfSize = Vector3(0.005f, 0.005f, 0.075f);
					const Vector3 rayPosition = Math::Forward() * (radius * 1.25f + rayHalfSize.z + 0.1f);
					const Reference<TriMesh> ray = GenerateMesh::Tri::Box(rayPosition - rayHalfSize, rayPosition + rayHalfSize);

					const Reference<TriMesh> ray0 = [&]() {
						Matrix4 transform = Math::Identity();
						transform[3] = Vector4(-0.1, 0.0f, -0.025f, 1.0f);
						return ModifyMesh::Transform(ray, transform);
					}();

					const Reference<TriMesh> ray180 = ModifyMesh::Transform(ray0, Math::MatrixFromEulerAngles(Vector3(0.0f, 0.0f, 180.0f)));
					const Reference<TriMesh> ray0_180 = ModifyMesh::Merge(ray0, ray180, "ray0_180");
					const Reference<TriMesh> ray90_270 = ModifyMesh::Transform(ray0_180, Math::MatrixFromEulerAngles(Vector3(0.0f, 0.0f, 90.0f)), "ray90_270");

					const Reference<TriMesh> ray90s = ModifyMesh::Merge(ray0_180, ray90_270, "ray90s");
					const Reference<TriMesh> ray45s = ModifyMesh::Transform(ray90s, Math::MatrixFromEulerAngles(Vector3(0.0f, 0.0f, 45.0f)), "ray45s");

					const Reference<TriMesh> sideRays = ModifyMesh::Merge(ray90s, ray45s, "sideRays");
					const Reference<TriMesh> allRays = ModifyMesh::Merge(ray, sideRays, "allRays");
					const Reference<TriMesh> shape =  ModifyMesh::Merge(rotatedCenter, allRays, "DirectionalLight");
					{
						TriMesh::Writer writer(shape);
						for (uint32_t i = 0; i < writer.VertCount(); i++) {
							MeshVertex& vertex = writer.Vert(i);
							const float factor = Math::Lerp(0.25f, 2.5f, vertex.position.z);
							vertex.position.x *= factor;
							vertex.position.y *= factor;
						}
					}
					return shape;
				}();
				return LIGHT_SHAPE;
			}
		};

		SpotLightGizmo::SpotLightGizmo(Scene::LogicContext* context) 
			: Component(context, "SpotLightGizmo")
			, m_handle(Object::Instantiate<Transform>(this, "SpotLightGizmo_Handle")) {
			Object::Instantiate<MeshRenderer>(m_handle, "PointLightGizmo_Renderer", Helpers::LightShape())->SetLayer(static_cast<Layer>(GizmoLayers::SELECTION_OVERLAY));
		}

		SpotLightGizmo::~SpotLightGizmo() {}

		void SpotLightGizmo::Update() {
			Component* target = TargetComponent();
			if (target == nullptr) return;
			Transform* targetTransform = target->GetTransfrom();
			if (targetTransform != nullptr && target->ActiveInHeirarchy()) {
				m_handle->SetEnabled(true);
				m_handle->SetWorldPosition(targetTransform->WorldPosition());
				m_handle->SetWorldEulerAngles(targetTransform->WorldEulerAngles());
			}
			else m_handle->SetEnabled(false);
		}

		namespace {
			static const constexpr Gizmo::ComponentConnection SpotLightGizmo_Connection =
				Gizmo::ComponentConnection::Make<SpotLightGizmo, SpotLight>(
					Gizmo::FilterFlag::CREATE_IF_SELECTED |
					Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED |
					Gizmo::FilterFlag::CREATE_CHILD_GIZMOS_IF_SELECTED |
					Gizmo::FilterFlag::CREATE_PARENT_GIZMOS_IF_SELECTED);
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::SpotLightGizmo>() {
		Editor::Gizmo::AddConnection(Editor::SpotLightGizmo_Connection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::SpotLightGizmo>() {
		Editor::Gizmo::RemoveConnection(Editor::SpotLightGizmo_Connection);
	}
}

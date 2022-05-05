#include "CameraGizmo.h"
#include <Data/Generators/MeshGenerator.h>
#include <Data/Generators/MeshModifiers.h>
#include <Components/Camera.h>
#include <Components/GraphicsObjects/MeshRenderer.h>


namespace Jimara {
	namespace Editor {
		namespace {
			static const Reference<TriMesh> CAMERA_SHAPE = []()->Reference<TriMesh> {

				const Vector3 boxHalfSize(0.05f, 0.075f, 0.15f);
				const Reference<TriMesh> body = GenerateMesh::Tri::Box(-boxHalfSize, boxHalfSize);

				const Reference<TriMesh> lense = [&]() {
					const float lenseRadius = 0.05f;
					const float lenseHeight = 0.1f;
					const Reference<TriMesh> cylinder = GenerateMesh::Tri::Cylinder(
						Vector3(0.0f, boxHalfSize.z - 0.0001f + lenseHeight * 0.75f, 0.0f), lenseRadius, lenseHeight * 0.5f, 24);
					const Reference<TriMesh> capsule = GenerateMesh::Tri::Capsule(
						Vector3(0.0f, boxHalfSize.z - 0.0001f + lenseHeight * 0.4f, 0.0f), lenseRadius * 0.75f, lenseHeight * 0.9f, 16, 4);
					const Reference<TriMesh> cylinderAndCapsule = ModifyMesh::Merge(cylinder, capsule, "cylinderAndCapsule");
					const Matrix4 forwardRotation = Math::MatrixFromEulerAngles(Vector3(90.0f, 0.0f, 0.0f));
					const Reference<TriMesh> transformedCylinder = ModifyMesh::Transform(cylinderAndCapsule, forwardRotation);
					
					const Vector3 rectHalfSize(lenseRadius * 1.5f, 0.001f, 0.05f);
					const Vector3 rectCenter = Math::Forward() * rectHalfSize.z;
					const Reference<TriMesh> rect = GenerateMesh::Tri::Box(rectCenter - rectHalfSize, rectCenter + rectHalfSize);
					{
						const TriMesh::Writer writer(rect);
						for (uint32_t i = 0; i < writer.VertCount(); i++) {
							MeshVertex& vertex = writer.Vert(i);
							vertex.position.x *= Math::Lerp(0.35f, 1.0f, vertex.position.z / (2.0f * rectHalfSize.z));
						}
					}

					Matrix4 rectTilt = Math::MatrixFromEulerAngles(Vector3(-35.0f, 0.0f, 0.0f));
					rectTilt[3] = Vector4(Math::Up() * (lenseRadius * 0.75f) + Math::Forward() * (lenseHeight * 0.95f + boxHalfSize.z), 1.0f);
					const Reference<TriMesh> rectA = ModifyMesh::Transform(rect, rectTilt);
					const Reference<TriMesh> rectB = ModifyMesh::Transform(rectA, Math::MatrixFromEulerAngles(Vector3(0.0f, 0.0f, 90.0f)));

					const Reference<TriMesh> rectAB = ModifyMesh::Merge(rectA, rectB, "rectAB");
					const Reference<TriMesh> rectCD = ModifyMesh::Transform(rectAB, Math::MatrixFromEulerAngles(Vector3(0.0f, 0.0f, 180.0f)), "rectCD");
					const Reference<TriMesh> rects = ModifyMesh::Merge(rectAB, rectCD, "rects");

					return ModifyMesh::Merge(transformedCylinder, rects, "Lense");
				}();

				const Reference<TriMesh> bodyAndLense = ModifyMesh::Merge(body, lense, "BodyAndLense");

				const float tapeRadius = 0.1f;
				const float tapeWidth = 0.05f;
				const Reference<TriMesh> tapeA = [&]() {
					const Reference<TriMesh> innerCylinder = GenerateMesh::Tri::Cylinder(Vector3(0.0f), tapeRadius * 0.9f, tapeWidth, 24);
					const Reference<TriMesh> outerCylinder = GenerateMesh::Tri::Cylinder(Vector3(0.0f), tapeRadius, tapeWidth * 0.8f, 24);
					const Reference<TriMesh> tapeCylinders = ModifyMesh::Merge(innerCylinder, outerCylinder, "tapeCylinders");
					
					const Reference<TriMesh> outerOutline = GenerateMesh::Tri::Cylinder(Vector3(0.0f), tapeRadius * 0.175f, tapeWidth * 1.15f, 8);
					const Reference<TriMesh> axleCenter = GenerateMesh::Tri::Capsule(Vector3(0.0f), tapeRadius * 0.15f, tapeWidth, 16, 4);
					const Reference<TriMesh> axle = ModifyMesh::Merge(outerOutline, axleCenter, "axle");

					const Reference<TriMesh> tapeShape = ModifyMesh::Merge(tapeCylinders, axle, "tapeShape");
					Matrix4 transform = Math::MatrixFromEulerAngles(Vector3(0.0f, 0.0f, 90.0f));
					transform[3] = Vector4(-Math::Forward() * tapeRadius + Math::Up() * (boxHalfSize.y + tapeRadius - 0.001f), 1.0f);
					return ModifyMesh::Transform(tapeShape, transform);
				}();

				const Reference<TriMesh> tapeB = [&]() {
					Matrix4 transform = Math::Identity();
					const float scale = 1.1f;
					transform[0] *= scale;
					transform[1] *= scale;
					transform[2] *= scale;
					transform[3] = Vector4(Math::Forward() * tapeRadius * scale * 2.0f + Math::Up() * 0.0125f, 1.0f);
					return ModifyMesh::Transform(tapeA, transform);
				}();

				const Reference<TriMesh> tapes = [&]() {
					const Vector3 connectionShapeHalfSize(0.015f, 0.05f, 0.075f);
					const Vector3 connectionShapeCenter(0.0f, boxHalfSize.y + connectionShapeHalfSize.y - 0.001f, 0.025f);
					const Reference<TriMesh> connectionShape = GenerateMesh::Tri::Box(
						connectionShapeCenter - connectionShapeHalfSize, 
						connectionShapeCenter + connectionShapeHalfSize);
					const Reference<TriMesh> tapeAB = ModifyMesh::Merge(tapeA, tapeB, "tapeAB");
					return ModifyMesh::Merge(tapeAB, connectionShape, "Tapes");
				}();

				return ModifyMesh::Merge(bodyAndLense, tapes, "Camera");
			}();
		}

		CameraGizmo::CameraGizmo(Scene::LogicContext* context) 
			: Component(context, "CameraGizmo")
			, m_handle(Object::Instantiate<Transform>(this, "CameraGizmo")) {
			Object::Instantiate<MeshRenderer>(m_handle, "CameraGizmo_Renderer", CAMERA_SHAPE)->SetLayer(static_cast<GraphicsLayer>(GizmoLayers::SELECTION_OVERLAY));
		}

		CameraGizmo::~CameraGizmo() {}

		void CameraGizmo::Update() {
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
			static const constexpr Gizmo::ComponentConnection TransformGizmo_Connection =
				Gizmo::ComponentConnection::Make<CameraGizmo, Camera>(
					Gizmo::FilterFlag::CREATE_IF_SELECTED |
					Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED |
					Gizmo::FilterFlag::CREATE_CHILD_GIZMOS_IF_SELECTED |
					Gizmo::FilterFlag::CREATE_PARENT_GIZMOS_IF_SELECTED);
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::CameraGizmo>() {
		Editor::Gizmo::AddConnection(Editor::TransformGizmo_Connection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::CameraGizmo>() {
		Editor::Gizmo::RemoveConnection(Editor::TransformGizmo_Connection);
	}
}

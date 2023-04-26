#include "SpotLightGizmo.h"
#include <Components/Lights/SpotLight.h>
#include <Data/Generators/MeshGenerator.h>
#include <Data/Generators/MeshModifiers.h>
#include <Data/Generators/MeshConstants.h>
#include <Data/Materials/SampleDiffuse/SampleDiffuseShader.h>
#include <Components/GraphicsObjects/MeshRenderer.h>
#include "../../../Handles/Compound/CircleResizeHandle.h"


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
			class SpotLightResizeHandle : public virtual Gizmo, public virtual SceneContext::UpdatingComponent {
			private:
				const Reference<CircleResizeHandle> m_resizeHandleInner;
				const Reference<CircleResizeHandle> m_resizeHandleOuter;
				const Reference<DragHandle> m_rangeHandle;
				const Reference<Transform> m_innerOutline;
				const Reference<Transform> m_outerOutline;

				static TriMesh* ConeOutline() {
					static Reference<TriMesh> shape = []() {
						Reference<TriMesh> mesh = Object::Instantiate<TriMesh>("SpotLightResizeHandle_ConeOutline");
						TriMesh::Writer writer(mesh);
						writer.AddVert(MeshVertex(Vector3(0.0f), Math::Back(), Vector2(0.0f)));
						auto addEdge = [&](Vector3 direction) {
							writer.AddFace(TriangleFace(0u, writer.VertCount(), 0u));
							writer.AddVert(MeshVertex(direction + Math::Forward(), direction, Vector2(0.0f)));
						};
						addEdge(Math::Right());
						addEdge(Math::Up());
						addEdge(Math::Left());
						addEdge(Math::Down());
						return mesh;
					}();
					return shape;
				};

				static const constexpr Vector3 HandleColor() { return Vector3(1.0f, 1.0f, 0.0f); }

			public:
				inline SpotLightResizeHandle(Scene::LogicContext* context)
					: Component(context, "PointLightResizeHandle")
					, m_resizeHandleInner(Object::Instantiate<CircleResizeHandle>(this, HandleColor()))
					, m_resizeHandleOuter(Object::Instantiate<CircleResizeHandle>(this, HandleColor()))
					, m_rangeHandle(Object::Instantiate<DragHandle>(this, "PointLightResizeHandle_Range", DragHandle::Flags::DRAG_Z))
					, m_innerOutline(Object::Instantiate<Transform>(this, "PointLightResizeHandle_InnerOutline"))
					, m_outerOutline(Object::Instantiate<Transform>(this, "PointLightResizeHandle_OuterOutline")) {
					m_resizeHandleInner->SetEnabled(false);
					m_resizeHandleOuter->SetEnabled(false);
					const Reference<Material::Instance> material = SampleDiffuseShader::MaterialInstance(Context()->Graphics()->Device(), HandleColor());
					auto addOutlineRenderer = [&](Transform* outline) {
						outline->SetEnabled(false);
						const Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(outline, outline->Name() + "_Renderer", ConeOutline());
						renderer->SetMaterialInstance(material);
						renderer->SetLayer(static_cast<Layer>(GizmoLayers::OVERLAY));
						renderer->SetGeometryType(Graphics::Experimental::GraphicsPipeline::IndexType::EDGE);
					};
					//addOutlineRenderer(m_innerOutline);
					addOutlineRenderer(m_outerOutline);
					{
						m_rangeHandle->SetEnabled(false);
						const Reference<TriMesh> mesh = MeshConstants::Tri::Cube();
						const Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(m_rangeHandle, m_rangeHandle->Name() + "_Renderer", mesh);
						renderer->SetMaterialInstance(material);
						renderer->SetLayer(static_cast<Layer>(GizmoLayers::HANDLE));
					}
				}

				inline virtual ~SpotLightResizeHandle() {}

				inline virtual void Update() override {
					Reference<SpotLight> target = Target<SpotLight>();
					if (target == nullptr) return;
					Transform* targetTransform = target->GetTransfrom();
					if (targetTransform != nullptr && target->ActiveInHeirarchy()) {
						const Matrix4 rotation = targetTransform->WorldRotationMatrix();
						const Vector3 position = targetTransform->WorldPosition();
						const Vector3 eulerAngles = targetTransform->WorldEulerAngles();
						const Vector3 forward = rotation[2];

						const float range = target->Range();
						const float invRange = 1.0f / Math::Max(std::numeric_limits<float>::epsilon(), range);
						const float innerAngle = target->InnerAngle();
						const float outerAngle = target->OuterAngle();

						{
							m_rangeHandle->SetEnabled(true);
							m_rangeHandle->SetWorldPosition(position + forward * range);
							m_rangeHandle->SetWorldEulerAngles(eulerAngles);
							m_rangeHandle->SetLocalScale(Vector3(GizmoContext()->Viewport()->GizmoSizeAt(m_rangeHandle->WorldPosition()) * 0.1f));
							target->SetRange(range + Math::Dot(m_rangeHandle->Delta(), forward));
						}

						auto updateHandle = [&](CircleResizeHandle* handle, Transform* outline, float angle) {
							float radius = std::tan(Math::Radians(angle)) * range;
							
							outline->SetEnabled(true);
							outline->SetWorldPosition(position);
							outline->SetWorldEulerAngles(eulerAngles);
							outline->SetLocalScale(Vector3(radius, radius, range));
							
							handle->SetEnabled(true);
							handle->Update(position + forward * range, eulerAngles, radius);
							return std::abs(Math::Degrees(std::atan(radius * invRange)));
						};

						target->SetInnerAngle(updateHandle(m_resizeHandleInner, m_innerOutline, innerAngle));
						target->SetOuterAngle(updateHandle(m_resizeHandleOuter, m_outerOutline, outerAngle));
					}
					else {
						m_resizeHandleInner->SetEnabled(false);
						m_resizeHandleOuter->SetEnabled(false);
						m_rangeHandle->SetEnabled(false);
						m_innerOutline->SetEnabled(false);
						m_outerOutline->SetEnabled(false);
					}
				}
			};

			static const constexpr Gizmo::ComponentConnection SpotLightGizmo_Connection =
				Gizmo::ComponentConnection::Make<SpotLightGizmo, SpotLight>(
					Gizmo::FilterFlag::CREATE_IF_SELECTED |
					Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED |
					Gizmo::FilterFlag::CREATE_CHILD_GIZMOS_IF_SELECTED |
					Gizmo::FilterFlag::CREATE_PARENT_GIZMOS_IF_SELECTED);
			static const constexpr Gizmo::ComponentConnection SpotLightResizeHandle_Connection =
				Gizmo::ComponentConnection::Make<SpotLightResizeHandle, SpotLight>();
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::SpotLightGizmo>() {
		Editor::Gizmo::AddConnection(Editor::SpotLightGizmo_Connection);
		Editor::Gizmo::AddConnection(Editor::SpotLightResizeHandle_Connection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::SpotLightGizmo>() {
		Editor::Gizmo::RemoveConnection(Editor::SpotLightGizmo_Connection);
		Editor::Gizmo::RemoveConnection(Editor::SpotLightResizeHandle_Connection);
	}
}

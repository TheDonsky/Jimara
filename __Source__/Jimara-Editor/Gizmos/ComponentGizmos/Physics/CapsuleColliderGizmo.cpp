#include "CapsuleColliderGizmo.h"
#include <Components/Physics/CapsuleCollider.h>
#include <Data/Generators/MeshConstants.h>
#include <Data/Materials/SampleDiffuse/SampleDiffuseShader.h>


namespace Jimara {
	namespace Editor {
		struct CapsuleColliderGizmo::Helpers {
			inline static void InitializeHandle(DragHandle* handle) {
				const Reference<TriMesh> shape = MeshContants::Tri::Cube();
				const Reference<Material::Instance> material = SampleDiffuseShader::MaterialInstance(
					handle->Context()->Graphics()->Device(), Vector3(0.0f, 1.0f, 0.0f));
				const Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(handle, "BoxColliderGizmo_HendleRenderer", shape);
				renderer->SetMaterialInstance(material);
				renderer->SetLayer(static_cast<GraphicsLayer>(GizmoLayers::HANDLE));
			}

			template<typename Action>
			static void ForAllHandles(RadiusHandles& handles, const Action& action) {
				{
					const constexpr Vector3 right = Math::Right();
					action(handles.right, right);
					action(handles.left, -right);
				}
				{
					const constexpr Vector3 forward = Math::Forward();
					action(handles.front, forward);
					action(handles.back, -forward);
				}
			}

			template<typename Action>
			static void ForAllHandles(HeightHandles& handles, const Action& action) {
				const constexpr Vector3 up = Math::Up();
				action(handles.top, up);
				action(handles.bottom, -up);
			}

			template<typename Action>
			static void ForAllHandles(CapsuleColliderGizmo* self, const Action& action) {
				ForAllHandles(self->m_heightHandles, action);
				ForAllHandles(self->m_topRadiusHandles, action);
				ForAllHandles(self->m_bottomRadiusHandles, action);
			}

			inline static void UpdateRenderer(CapsuleColliderGizmo* self) {
				Reference<CapsuleCollider> collider = self->Target<CapsuleCollider>();
				Reference<Transform> colliderTransform = (collider == nullptr ? nullptr : collider->GetTransfrom());
				if (colliderTransform == nullptr || (!collider->ActiveInHeirarchy()))
					self->m_renderer->SetEnabled(false);
				else {
					self->m_renderer->SetEnabled(true);

					// Scale:
					const Vector3 lossyScale = colliderTransform->LossyScale();
					float tipScale = Math::Max(lossyScale.x, Math::Max(lossyScale.y, lossyScale.z));

					// Update mesh:
					{
						float radius = collider->Radius() * tipScale;
						float height = collider->Height() * lossyScale.y;
						if (std::abs(radius) > std::numeric_limits<float>::epsilon()) {
							height /= radius;
							tipScale *= collider->Radius();
							radius = 1.0f;
						}
						if (self->m_lastRadius != radius || self->m_lastHeight != height) {
							Reference<TriMesh> mesh = MeshContants::Tri::WireCapsule(radius, height);
							self->m_renderer->SetMesh(mesh);
							self->m_lastRadius = radius;
							self->m_lastHeight = height;
						}
					}

					// Update transform:
					{
						Transform* poseTransform = self->m_renderer->GetTransfrom();
						poseTransform->SetWorldPosition(colliderTransform->WorldPosition());
						poseTransform->SetWorldEulerAngles(Math::EulerAnglesFromMatrix(colliderTransform->WorldRotationMatrix() * [&]() -> Matrix4 {
							Physics::CapsuleShape::Alignment alignment = collider->Alignment();
							if (alignment == Physics::CapsuleShape::Alignment::X) return Math::MatrixFromEulerAngles(Vector3(0.0f, 0.0f, 90.0f));
							else if (alignment == Physics::CapsuleShape::Alignment::Y) return Math::Identity();
							else if (alignment == Physics::CapsuleShape::Alignment::Z) return Math::MatrixFromEulerAngles(Vector3(90.0f, 0.0f, 0.0f));
							else return Math::Identity();
							}()));
						poseTransform->SetLocalScale(std::abs(self->m_lastRadius) > std::numeric_limits<float>::epsilon() ? Vector3(tipScale) : lossyScale);
					}
				}
			}

			inline static void PoseHandles(CapsuleColliderGizmo* self) {
				const Transform* const poseTransform = self->m_renderer->GetTransfrom();
				if (poseTransform != nullptr && self->m_renderer->Enabled()) {
					const constexpr float BASE_HANDLE_SIZE = 0.05f;
					const Vector3 basePosition = poseTransform->LocalPosition();
					const Vector3 rotation = poseTransform->LocalEulerAngles();
					const Vector3 size = poseTransform->LocalScale();
					const Vector3 directionScale = [&] {
						auto sign = [&](float f) { return f >= 0.0f ? 1.0f : -1.0f; };
						return Vector3(sign(size.x), sign(size.y), sign(size.z));
					}();
					Reference<CapsuleCollider> collider = self->Target<CapsuleCollider>();
					const float capsuleHeight = self->m_lastHeight * size.y;
					
					// Pose radial handles:
					{
						auto poseRadiusHandles = [&](DragHandle* handle, Vector3 localDirection, float direction) {
							handle->SetEnabled(true);
							handle->SetLocalEulerAngles(rotation);
							localDirection *= directionScale;
							const Vector3 worldDirection = handle->LocalToWorldDirection(localDirection);
							const Vector3 position = basePosition +
								(handle->Up() * (direction * capsuleHeight * 0.5f)) +
								(worldDirection * std::abs(Math::Dot(localDirection, size)));
							handle->SetLocalPosition(position);
							const float scaleMultiplier = self->GizmoContext()->Viewport()->GizmoSizeAt(position);
							handle->SetLocalScale(Vector3(BASE_HANDLE_SIZE * scaleMultiplier));
						};
						ForAllHandles(self->m_topRadiusHandles, [&](DragHandle* handle, const Vector3& localDirection) {
							poseRadiusHandles(handle, localDirection, 1.0f);
							});
						ForAllHandles(self->m_bottomRadiusHandles, [&](DragHandle* handle, const Vector3& localDirection) {
							poseRadiusHandles(handle, localDirection, -1.0f);
							});
					}

					// Pose height handles:
					{
						auto poseHeightHandle = [&](DragHandle* handle, Vector3 localDirection) {
							handle->SetEnabled(true);
							handle->SetLocalEulerAngles(rotation);
							localDirection *= directionScale;
							const Vector3 worldDirection = handle->LocalToWorldDirection(localDirection);
							const Vector3 position = basePosition +
								(worldDirection * ((capsuleHeight * 0.5f) + std::abs(Math::Dot(localDirection, size))));
							handle->SetLocalPosition(position);
							const float scaleMultiplier = self->GizmoContext()->Viewport()->GizmoSizeAt(position);
							handle->SetLocalScale(Vector3(BASE_HANDLE_SIZE * scaleMultiplier));
						};
						ForAllHandles(self->m_heightHandles, poseHeightHandle);
					}
				}
				else ForAllHandles(self, [&](DragHandle* handle, const auto&) { handle->SetEnabled(false); });
			}

			inline static void DragHandles(CapsuleColliderGizmo* self) {
				const Transform* const poseTransform = self->m_renderer->GetTransfrom();
				if (poseTransform == nullptr || (!self->m_renderer->Enabled())) return;
				Reference<CapsuleCollider> collider = self->Target<CapsuleCollider>();
				float radius = collider->Radius();
				float height = collider->Height();
				const Vector3 invScale = [&]() {
					auto inverseScale = [&](float a, float b) {
						return a * ((std::abs(b) > std::numeric_limits<float>::epsilon()) ? (1.0f / b) : 0.0f);
					};
					const Vector3 totalScale = poseTransform->LocalScale();
					return Vector3(inverseScale(radius, totalScale.x), inverseScale(radius, totalScale.y), inverseScale(radius, totalScale.z));
				}();

				auto dragHandle = [&](DragHandle* handle, const Vector3& localDirection, float& value, float multiplier) {
					if (!handle->HandleActive()) return;
					const Vector3 worldDelta = handle->Delta();
					const Vector3 worldDirection = handle->LocalToWorldDirection(localDirection);
					const float deltaAmount = Math::Dot(worldDirection, worldDelta);
					const float scaledAmount = deltaAmount * Math::Dot(localDirection, invScale);
					value += scaledAmount * Math::Dot(localDirection, Vector3(1.0f)) * multiplier;
				};

				// Drag radius:
				{
					auto dragRadiusHandle = [&](DragHandle* handle, const Vector3& localDirection) {
						dragHandle(handle, localDirection, radius, 1.0f);
					};
					ForAllHandles(self->m_topRadiusHandles, dragRadiusHandle);
					ForAllHandles(self->m_bottomRadiusHandles, dragRadiusHandle);
					collider->SetRadius(radius);
				}

				// Drag height:
				{
					auto dragHeightHandle = [&](DragHandle* handle, const Vector3& localDirection) {
						dragHandle(handle, localDirection, height, radius > 0.0f ? 2.0f : -2.0f);
					};
					ForAllHandles(self->m_heightHandles, dragHeightHandle);
					collider->SetHeight(height);
				}
			}
		};

		CapsuleColliderGizmo::RadiusHandles::RadiusHandles(CapsuleColliderGizmo* parent) 
			: right(Object::Instantiate<DragHandle>(parent, "CapsuleColliderGizmo_RadiusHandle_Right", DragHandle::Flags::DRAG_X))
			, left(Object::Instantiate<DragHandle>(parent, "CapsuleColliderGizmo_RadiusHandle_Left", DragHandle::Flags::DRAG_X))
			, front(Object::Instantiate<DragHandle>(parent, "CapsuleColliderGizmo_RadiusHandle_Front", DragHandle::Flags::DRAG_Z))
			, back(Object::Instantiate<DragHandle>(parent, "CapsuleColliderGizmo_RadiusHandle_Back", DragHandle::Flags::DRAG_Z)) {
			Helpers::InitializeHandle(right);
			Helpers::InitializeHandle(left);
			Helpers::InitializeHandle(front);
			Helpers::InitializeHandle(back);
		}

		CapsuleColliderGizmo::HeightHandles::HeightHandles(CapsuleColliderGizmo* parent) 
			: top(Object::Instantiate<DragHandle>(parent, "CapsuleColliderGizmo_HeightHandle_Top", DragHandle::Flags::DRAG_Y))
			, bottom(Object::Instantiate<DragHandle>(parent, "CapsuleColliderGizmo_HeightHandle_Bottom", DragHandle::Flags::DRAG_Y)) {
			Helpers::InitializeHandle(top);
			Helpers::InitializeHandle(bottom);
		}

		CapsuleColliderGizmo::CapsuleColliderGizmo(Scene::LogicContext* context)
			: Component(context, "CapsuleColliderGizmo")
			, m_renderer(Object::Instantiate<MeshRenderer>(
				Object::Instantiate<Transform>(this, "CapsuleColliderGizmo_Pose"), "CapsuleColliderGizmo_ShapeRenderer"))
			, m_heightHandles(this)
			, m_topRadiusHandles(this)
			, m_bottomRadiusHandles(this) {
			const Reference<Material::Instance> material = SampleDiffuseShader::MaterialInstance(Context()->Graphics()->Device(), Vector3(0.0f, 1.0f, 0.0f));
			m_renderer->SetMaterialInstance(material);
			m_renderer->SetLayer(static_cast<GraphicsLayer>(GizmoLayers::OVERLAY));
			m_renderer->SetGeometryType(Graphics::GraphicsPipeline::IndexType::EDGE);
		}

		CapsuleColliderGizmo::~CapsuleColliderGizmo() {}

		void CapsuleColliderGizmo::Update() {
			Helpers::UpdateRenderer(this);
			Helpers::PoseHandles(this);
			Helpers::DragHandles(this);
		}

		namespace {
			static const constexpr Gizmo::ComponentConnection ColliderGizmo_Connection =
				Gizmo::ComponentConnection::Make<CapsuleColliderGizmo, CapsuleCollider>();
		}
	}


	template<> void TypeIdDetails::OnRegisterType<Editor::CapsuleColliderGizmo>() {
		Editor::Gizmo::AddConnection(Editor::ColliderGizmo_Connection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::CapsuleColliderGizmo>() {
		Editor::Gizmo::RemoveConnection(Editor::ColliderGizmo_Connection);
	}
}

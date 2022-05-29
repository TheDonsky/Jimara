#include "SphereColliderGizmo.h"
#include <Components/GraphicsObjects/MeshRenderer.h>
#include <Components/Physics/SphereCollider.h>
#include <Data/Generators/MeshConstants.h>
#include <Data/Materials/SampleDiffuse/SampleDiffuseShader.h>


namespace Jimara {
	namespace Editor {
		struct SphereColliderGizmo::Helpers {
			template<typename ActionType>
			inline static void ForAllHandles(SphereColliderGizmo* self, const ActionType& action) {
				{
					const constexpr Vector3 right = Math::Right();
					action(self->m_resizeRight, right);
					action(self->m_resizeLeft, -right);
				}
				{
					const constexpr Vector3 up = Math::Up();
					action(self->m_resizeUp, up);
					action(self->m_resizeDown, -up);
				}
				{
					const constexpr Vector3 forward = Math::Forward();
					action(self->m_resizeFront, forward);
					action(self->m_resizeBack, -forward);
				}
			}

			inline static void PoseShape(SphereColliderGizmo* self) {
				Reference<SphereCollider> collider = self->Target<SphereCollider>();
				Reference<Transform> colliderTransform = (collider == nullptr ? nullptr : collider->GetTransfrom());
				if (colliderTransform == nullptr || (!collider->ActiveInHeirarchy()))
					self->m_poseTransform->SetEnabled(false);
				else {
					self->m_poseTransform->SetEnabled(true);
					self->m_poseTransform->SetWorldPosition(colliderTransform->WorldPosition());
					self->m_poseTransform->SetWorldEulerAngles(colliderTransform->WorldEulerAngles());
					Vector3 objectScale = colliderTransform->LossyScale();
					self->m_poseTransform->SetLocalScale(Vector3(Math::Max(objectScale.x, Math::Max(objectScale.y, objectScale.z)) * collider->Radius()));
				}
			}

			inline static void PoseHandles(SphereColliderGizmo* self) {
				if (self->m_poseTransform->Enabled()) {
					const constexpr float BASE_HANDLE_SIZE = 0.05f;
					const Vector3 basePosition = self->m_poseTransform->LocalPosition();
					const Vector3 rotation = self->m_poseTransform->LocalEulerAngles();
					const Vector3 size = self->m_poseTransform->LocalScale();
					const Vector3 directionScale = [&] {
						auto sign = [&](float f) { return f >= 0.0f ? 1.0f : -1.0f; };
						return Vector3(sign(size.x), sign(size.y), sign(size.z));
					}();
					ForAllHandles(self, [&](DragHandle* handle, Vector3 localDirection) {
						handle->SetEnabled(true);
						handle->SetLocalEulerAngles(rotation);
						localDirection *= directionScale;
						const Vector3 worldDirection = handle->LocalToWorldDirection(localDirection);
						const Vector3 position = basePosition + (worldDirection * std::abs(Math::Dot(localDirection, size)));
						handle->SetLocalPosition(position);
						const float scaleMultiplier = self->GizmoContext()->Viewport()->GizmoSizeAt(position);
						handle->SetLocalScale(Vector3(BASE_HANDLE_SIZE * scaleMultiplier));
						});
				}
				else ForAllHandles(self, [&](DragHandle* handle, const auto&) { handle->SetEnabled(false); });
			};

			inline static void DragHandles(SphereColliderGizmo* self) {
				if (!self->m_poseTransform->Enabled()) return;
				Reference<SphereCollider> collider = self->Target<SphereCollider>();
				float radius = collider->Radius();
				const Vector3 invScale = [&]() {
					auto inverseScale = [&](float a, float b) {
						return a * ((std::abs(b) > std::numeric_limits<float>::epsilon()) ? (1.0f / b) : 0.0f);
					};
					const Vector3 totalScale = self->m_poseTransform->LocalScale();
					return Vector3(inverseScale(radius, totalScale.x), inverseScale(radius, totalScale.y), inverseScale(radius, totalScale.z));
				}();
				ForAllHandles(self, [&](DragHandle* handle, const Vector3& localDirection) {
					if (!handle->HandleActive()) return;
					const Vector3 worldDelta = handle->Delta();
					const Vector3 worldDirection = handle->LocalToWorldDirection(localDirection);
					const float deltaAmount = Math::Dot(worldDirection, worldDelta);
					const float scaledAmount = deltaAmount * Math::Dot(localDirection, invScale);
					radius += scaledAmount * Math::Dot(localDirection, Vector3(1.0f));
					});
				collider->SetRadius(radius);
			};

			inline static void InitializeRenderers(SphereColliderGizmo* self) {
				const Reference<TriMesh> shape = MeshContants::Tri::Cube();
				const Reference<Material::Instance> material = SampleDiffuseShader::MaterialInstance(
					self->Context()->Graphics()->Device(), Vector3(0.0f, 1.0f, 0.0f));
				ForAllHandles(self, [&](DragHandle* handle, const Vector3& localDirection) {
					const Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(handle, "BoxColliderGizmo_HendleRenderer", shape);
					renderer->SetMaterialInstance(material);
					renderer->SetLayer(static_cast<GraphicsLayer>(GizmoLayers::HANDLE));
					});
			}
		};

		SphereColliderGizmo::SphereColliderGizmo(Scene::LogicContext* context)
			: Component(context, "SphereColliderGizmo")
			, m_poseTransform(Object::Instantiate<Transform>(this, "SphereColliderGizmo_Pose"))
			, m_resizeRight(Object::Instantiate<DragHandle>(this, "SphereColliderGizmo_ResizeRight", DragHandle::Flags::DRAG_X))
			, m_resizeLeft(Object::Instantiate<DragHandle>(this, "SphereColliderGizmo_ResizeLeft", DragHandle::Flags::DRAG_X))
			, m_resizeUp(Object::Instantiate<DragHandle>(this, "SphereColliderGizmo_ResizeUp", DragHandle::Flags::DRAG_Y))
			, m_resizeDown(Object::Instantiate<DragHandle>(this, "SphereColliderGizmo_ResizeDown", DragHandle::Flags::DRAG_Y))
			, m_resizeFront(Object::Instantiate<DragHandle>(this, "SphereColliderGizmo_ResizeFront", DragHandle::Flags::DRAG_Z))
			, m_resizeBack(Object::Instantiate<DragHandle>(this, "SphereColliderGizmo_ResizeBack", DragHandle::Flags::DRAG_Z)) {
			const Reference<TriMesh> shape = MeshContants::Tri::WireSphere();
			const Reference<Material::Instance> material = SampleDiffuseShader::MaterialInstance(Context()->Graphics()->Device(), Vector3(0.0f, 1.0f, 0.0f));
			const Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(m_poseTransform, "SphereColliderGizmo_ShapeRenderer", shape);
			renderer->SetMaterialInstance(material);
			renderer->SetLayer(static_cast<GraphicsLayer>(GizmoLayers::OVERLAY));
			renderer->SetGeometryType(Graphics::GraphicsPipeline::IndexType::EDGE);
			Helpers::InitializeRenderers(this);
		}

		SphereColliderGizmo::~SphereColliderGizmo() {}

		void SphereColliderGizmo::Update() {
			Helpers::PoseShape(this);
			Helpers::PoseHandles(this);
			Helpers::DragHandles(this);
		}

		namespace {
			static const constexpr Gizmo::ComponentConnection ColliderGizmo_Connection =
				Gizmo::ComponentConnection::Make<SphereColliderGizmo, SphereCollider>();
		}
	}


	template<> void TypeIdDetails::OnRegisterType<Editor::SphereColliderGizmo>() {
		Editor::Gizmo::AddConnection(Editor::ColliderGizmo_Connection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::SphereColliderGizmo>() {
		Editor::Gizmo::RemoveConnection(Editor::ColliderGizmo_Connection);
	}
}

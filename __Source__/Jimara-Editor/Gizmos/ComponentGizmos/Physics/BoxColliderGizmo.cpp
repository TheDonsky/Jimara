#include "BoxColliderGizmo.h"
#include <Components/GraphicsObjects/MeshRenderer.h>
#include <Components/Physics/BoxCollider.h>
#include <Data/Generators/MeshConstants.h>
#include <Data/Materials/SampleDiffuse/SampleDiffuseShader.h>


namespace Jimara {
	namespace Editor {
		struct BoxColliderGizmo::Helpers {
			template<typename ActionType>
			inline static void ForAllHandles(BoxColliderGizmo* self, const ActionType& action) {
				{
					const constexpr Vector3 right = Math::Right();
					action(self->m_resizeRight, right);
					action(self->m_resizeLeft, -right);
				}
				{
					const constexpr Vector3 up = Math::Up();
					action(self->m_resizeUp, up);
					action(self->m_resizeDown, - up);
				}
				{
					const constexpr Vector3 forward = Math::Forward();
					action(self->m_resizeFront, forward);
					action(self->m_resizeBack, -forward);
				}
			}

			inline static void PoseShape(BoxColliderGizmo* self) {
				Reference<BoxCollider> collider = self->Target<BoxCollider>();
				Reference<Transform> colliderTransform = (collider == nullptr ? nullptr : collider->GetTransfrom());
				if (colliderTransform == nullptr || (!collider->ActiveInHeirarchy()))
					self->m_poseTransform->SetEnabled(false);
				else {
					self->m_poseTransform->SetEnabled(true);
					self->m_poseTransform->SetWorldPosition(colliderTransform->WorldPosition());
					self->m_poseTransform->SetWorldEulerAngles(colliderTransform->WorldEulerAngles());
					self->m_poseTransform->SetLocalScale(colliderTransform->LossyScale() * collider->Size());
				}
			}

			inline static void PoseHandles(BoxColliderGizmo* self) {
				if (self->m_poseTransform->Enabled()) {
					const constexpr float BASE_HANDLE_SIZE = 0.05f;
					const Vector3 basePosition = self->m_poseTransform->LocalPosition();
					const Vector3 rotation = self->m_poseTransform->LocalEulerAngles();
					const Vector3 size = self->m_poseTransform->LocalScale();
					ForAllHandles(self, [&](DragHandle* handle, const Vector3& localDirection) {
						handle->SetEnabled(true);
						handle->SetLocalEulerAngles(rotation);
						const Vector3 worldDirection = handle->LocalToWorldDirection(localDirection);
						const Vector3 position = basePosition + (worldDirection * std::abs(Math::Dot(localDirection, size) * 0.5f));
						handle->SetLocalPosition(position);
						const float scaleMultiplier = self->GizmoContext()->Viewport()->GizmoSizeAt(position);
						handle->SetLocalScale(Vector3(BASE_HANDLE_SIZE * scaleMultiplier));
						});
				}
				else ForAllHandles(self, [&](DragHandle* handle, const auto&) { handle->SetEnabled(false); });
			};

			inline static void DragHandles(BoxColliderGizmo* self) {
				if (!self->m_poseTransform->Enabled()) return;
				Reference<BoxCollider> collider = self->Target<BoxCollider>();
				Vector3 size = collider->Size();
				const Vector3 invScale = [&]() {
					auto inverseScale = [&](float a, float b) {
						return std::abs(a * ((std::abs(b) > std::numeric_limits<float>::epsilon()) ? (1.0f / b) : 0.0f));
					};
					const Vector3 totalScale = self->m_poseTransform->LocalScale();
					return Vector3(inverseScale(size.x, totalScale.x), inverseScale(size.y, totalScale.y), inverseScale(size.z, totalScale.z));
				}();
				ForAllHandles(self, [&](DragHandle* handle, const Vector3& localDirection) {
					if (!handle->HandleActive()) return;
					const Vector3 worldDelta = handle->Delta();
					const Vector3 worldDirection = handle->LocalToWorldDirection(localDirection);
					const float deltaAmount = Math::Dot(worldDirection, worldDelta);
					const float scaledAmount = deltaAmount * Math::Dot(localDirection, invScale) * 2.0f;
					size += scaledAmount * localDirection;
					});
				collider->SetSize(size);
			};

			inline static void InitializeRenderers(BoxColliderGizmo* self) {
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

		BoxColliderGizmo::BoxColliderGizmo(Scene::LogicContext* context) 
			: Component(context, "BoxColliderGizmo")
			, m_poseTransform(Object::Instantiate<Transform>(this, "BoxColliderGizmo_Pose"))
			, m_resizeRight(Object::Instantiate<DragHandle>(this, "BoxColliderGizmo_ResizeRight", DragHandle::Flags::DRAG_X))
			, m_resizeLeft(Object::Instantiate<DragHandle>(this, "BoxColliderGizmo_ResizeLeft", DragHandle::Flags::DRAG_X))
			, m_resizeUp(Object::Instantiate<DragHandle>(this, "BoxColliderGizmo_ResizeUp", DragHandle::Flags::DRAG_Y))
			, m_resizeDown(Object::Instantiate<DragHandle>(this, "BoxColliderGizmo_ResizeDown", DragHandle::Flags::DRAG_Y))
			, m_resizeFront(Object::Instantiate<DragHandle>(this, "BoxColliderGizmo_ResizeFront", DragHandle::Flags::DRAG_Z))
			, m_resizeBack(Object::Instantiate<DragHandle>(this, "BoxColliderGizmo_ResizeBack", DragHandle::Flags::DRAG_Z)) {
			const Reference<TriMesh> shape = MeshContants::Tri::WireCube();
			const Reference<Material::Instance> material = SampleDiffuseShader::MaterialInstance(Context()->Graphics()->Device(), Vector3(0.0f, 1.0f, 0.0f));
			const Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(m_poseTransform, "BoxColliderGizmo_ShapeRenderer", shape);
			renderer->SetMaterialInstance(material);
			renderer->SetLayer(static_cast<GraphicsLayer>(GizmoLayers::OVERLAY));
			renderer->SetGeometryType(Graphics::GraphicsPipeline::IndexType::EDGE);
			Helpers::InitializeRenderers(this);
		}

		BoxColliderGizmo::~BoxColliderGizmo() {}

		void BoxColliderGizmo::Update() {
			Helpers::PoseShape(this);
			Helpers::PoseHandles(this);
			Helpers::DragHandles(this);
		}

		namespace {
			static const constexpr Gizmo::ComponentConnection ColliderGizmo_Connection =
				Gizmo::ComponentConnection::Make<BoxColliderGizmo, BoxCollider>();
		}
	}


	template<> void TypeIdDetails::OnRegisterType<Editor::BoxColliderGizmo>() {
		Editor::Gizmo::AddConnection(Editor::ColliderGizmo_Connection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::BoxColliderGizmo>() {
		Editor::Gizmo::RemoveConnection(Editor::ColliderGizmo_Connection);
	}
}

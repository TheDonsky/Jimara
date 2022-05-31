#include "BoxResizeHandle.h"
#include <Components/GraphicsObjects/MeshRenderer.h>
#include <Data/Generators/MeshConstants.h>
#include <Data/Materials/SampleDiffuse/SampleDiffuseShader.h>


namespace Jimara {
	namespace Editor {
		struct BoxResizeHandle::Helpers {
			template<typename ActionType>
			inline static void ForAllHandles(BoxResizeHandle* self, const ActionType& action) {
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

			inline static void PoseShape(BoxResizeHandle* self, const Vector3& position, const Vector3& rotation, const Vector3& size) {
				self->m_poseTransform->SetWorldPosition(position);
				self->m_poseTransform->SetWorldEulerAngles(rotation);
				self->m_poseTransform->SetLocalScale(size);
			}

			inline static void PoseHandles(BoxResizeHandle* self) {
				if (self->m_poseTransform->Enabled()) {
					const constexpr float BASE_HANDLE_SIZE = 0.05f;
					const Vector3 basePosition = self->m_poseTransform->WorldPosition();
					const Vector3 rotation = self->m_poseTransform->WorldEulerAngles();
					const Vector3 size = self->m_poseTransform->LossyScale();
					const Vector3 directionScale = [&] {
						auto sign = [&](float f) { return f >= 0.0f ? 1.0f : -1.0f; };
						return Vector3(sign(size.x), sign(size.y), sign(size.z));
					}();
					ForAllHandles(self, [&](DragHandle* handle, Vector3 localDirection) {
						handle->SetWorldEulerAngles(rotation);
						localDirection *= directionScale;
						const Vector3 worldDirection = handle->LocalToWorldDirection(localDirection);
						const Vector3 position = basePosition + (worldDirection * std::abs(Math::Dot(localDirection, size) * 0.5f));
						handle->SetWorldPosition(position);
						const float scaleMultiplier = handle->GizmoContext()->Viewport()->GizmoSizeAt(position);
						handle->SetLocalScale(Vector3(BASE_HANDLE_SIZE * scaleMultiplier));
						});
				}
			};

			inline static void DragHandles(BoxResizeHandle* self, Vector3& size) {
				if (!self->m_poseTransform->Enabled()) return;
				const Vector3 invScale = [&]() {
					auto inverseScale = [&](float a, float b) {
						return a * ((std::abs(b) > std::numeric_limits<float>::epsilon()) ? (1.0f / b) : 0.0f);
					};
					const Vector3 totalScale = self->m_poseTransform->LossyScale();
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
			};

			inline static void InitializeRenderers(BoxResizeHandle* self) {
				const Reference<TriMesh> shape = MeshContants::Tri::Cube();
				ForAllHandles(self, [&](DragHandle* handle, const Vector3& localDirection) {
					const Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(handle, "BoxResizeHandle_HandleRenderer", shape);
					renderer->SetMaterialInstance(self->m_poseTransform->GetComponentInChildren<MeshRenderer>()->MaterialInstance());
					renderer->SetLayer(static_cast<GraphicsLayer>(GizmoLayers::HANDLE));
					});
			}
		};

		BoxResizeHandle::BoxResizeHandle(Component* parent, const Vector3& color)
			: Component(parent, "BoxResizeHandle")
			, m_poseTransform(Object::Instantiate<Transform>(this, "BoxResizeHandle_Pose"))
			, m_resizeRight(Object::Instantiate<DragHandle>(this, "BoxResizeHandle_ResizeRight", DragHandle::Flags::DRAG_X))
			, m_resizeLeft(Object::Instantiate<DragHandle>(this, "BoxResizeHandle_ResizeLeft", DragHandle::Flags::DRAG_X))
			, m_resizeUp(Object::Instantiate<DragHandle>(this, "BoxResizeHandle_ResizeUp", DragHandle::Flags::DRAG_Y))
			, m_resizeDown(Object::Instantiate<DragHandle>(this, "BoxResizeHandle_ResizeDown", DragHandle::Flags::DRAG_Y))
			, m_resizeFront(Object::Instantiate<DragHandle>(this, "BoxResizeHandle_ResizeFront", DragHandle::Flags::DRAG_Z))
			, m_resizeBack(Object::Instantiate<DragHandle>(this, "BoxResizeHandle_ResizeBack", DragHandle::Flags::DRAG_Z)) {
			const Reference<TriMesh> shape = MeshContants::Tri::WireCube();
			const Reference<Material::Instance> material = SampleDiffuseShader::MaterialInstance(Context()->Graphics()->Device(), color);
			const Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(m_poseTransform, "BoxResizeHandle_ShapeRenderer", shape);
			renderer->SetMaterialInstance(material);
			renderer->SetLayer(static_cast<GraphicsLayer>(GizmoLayers::OVERLAY));
			renderer->SetGeometryType(Graphics::GraphicsPipeline::IndexType::EDGE);
			Helpers::InitializeRenderers(this);
		}

		BoxResizeHandle::~BoxResizeHandle() {}

		void BoxResizeHandle::Update(Vector3 position, Vector3 rotation, Vector3& size) {
			Helpers::PoseShape(this, position, rotation, size);
			Helpers::PoseHandles(this);
			Helpers::DragHandles(this, size);
		}
	}
}

#include "SphereResizeHandle.h"
#include <Components/GraphicsObjects/MeshRenderer.h>
#include <Data/Generators/MeshConstants.h>
#include <Data/Materials/SampleDiffuse/SampleDiffuseShader.h>

namespace Jimara {
	namespace Editor {
		struct SphereResizeHandle::Helpers {
			template<typename ActionType>
			inline static void ForAllHandles(SphereResizeHandle* self, const ActionType& action) {
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

			inline static void PoseShape(SphereResizeHandle* self, const Vector3& position, const Vector3& rotation, float radius) {
				self->m_poseTransform->SetWorldPosition(position);
				self->m_poseTransform->SetWorldEulerAngles(rotation);
				self->m_poseTransform->SetLocalScale(Vector3(std::abs(radius)));
			}

			inline static void PoseHandles(SphereResizeHandle* self, float radius) {
				if (self->m_poseTransform->Enabled()) {
					const constexpr float BASE_HANDLE_SIZE = 0.05f;
					const Vector3 basePosition = self->m_poseTransform->LocalPosition();
					const Vector3 rotation = self->m_poseTransform->LocalEulerAngles();
					ForAllHandles(self, [&](DragHandle* handle, Vector3 localDirection) {
						handle->SetEnabled(true);
						handle->SetLocalEulerAngles(rotation);
						const Vector3 worldDirection = handle->LocalToWorldDirection(localDirection);
						const Vector3 position = basePosition + (worldDirection * radius);
						handle->SetLocalPosition(position);
						const float scaleMultiplier = handle->GizmoContext()->Viewport()->GizmoSizeAt(position);
						handle->SetLocalScale(Vector3(BASE_HANDLE_SIZE * scaleMultiplier));
						});
				}
				else ForAllHandles(self, [&](DragHandle* handle, const auto&) { handle->SetEnabled(false); });
			};

			inline static void DragHandles(SphereResizeHandle* self, float& radius) {
				if (!self->m_poseTransform->Enabled()) return;
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
					radius += scaledAmount * Math::Dot(localDirection, Vector3(1.0f)) * ((radius >= 0.0f) ? 1.0f : -1.0f);
					});
			};

			inline static void InitializeRenderers(SphereResizeHandle* self) {
				const Reference<TriMesh> shape = MeshContants::Tri::Cube();
				ForAllHandles(self, [&](DragHandle* handle, const Vector3& localDirection) {
					const Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(handle, "SphereResizeHandle_HendleRenderer", shape);
					renderer->SetMaterialInstance(self->m_poseTransform->GetComponentInChildren<MeshRenderer>()->MaterialInstance());
					renderer->SetLayer(static_cast<GraphicsLayer>(GizmoLayers::HANDLE));
					});
			}
		};

		SphereResizeHandle::SphereResizeHandle(Component* parent, const Vector3& color)
			: Component(parent, "SphereResizeHandle")
			, m_poseTransform(Object::Instantiate<Transform>(this, "SphereResizeHandle_Pose"))
			, m_resizeRight(Object::Instantiate<DragHandle>(this, "SphereResizeHandle_ResizeRight", DragHandle::Flags::DRAG_X))
			, m_resizeLeft(Object::Instantiate<DragHandle>(this, "SphereResizeHandle_ResizeLeft", DragHandle::Flags::DRAG_X))
			, m_resizeUp(Object::Instantiate<DragHandle>(this, "SphereResizeHandle_ResizeUp", DragHandle::Flags::DRAG_Y))
			, m_resizeDown(Object::Instantiate<DragHandle>(this, "SphereResizeHandle_ResizeDown", DragHandle::Flags::DRAG_Y))
			, m_resizeFront(Object::Instantiate<DragHandle>(this, "SphereResizeHandle_ResizeFront", DragHandle::Flags::DRAG_Z))
			, m_resizeBack(Object::Instantiate<DragHandle>(this, "SphereResizeHandle_ResizeBack", DragHandle::Flags::DRAG_Z)) {
			const Reference<TriMesh> shape = MeshContants::Tri::WireSphere();
			const Reference<Material::Instance> material = SampleDiffuseShader::MaterialInstance(Context()->Graphics()->Device(), color);
			const Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(m_poseTransform, "SphereResizeHandle_ShapeRenderer", shape);
			renderer->SetMaterialInstance(material);
			renderer->SetLayer(static_cast<GraphicsLayer>(GizmoLayers::OVERLAY));
			renderer->SetGeometryType(Graphics::GraphicsPipeline::IndexType::EDGE);
			Helpers::InitializeRenderers(this);
		}

		SphereResizeHandle::~SphereResizeHandle() {}

		void SphereResizeHandle::Update(Vector3 position, Vector3 rotation, float& radius) {
			Helpers::PoseShape(this, position, rotation, radius);
			Helpers::PoseHandles(this, radius);
			Helpers::DragHandles(this, radius);
		}
	}
}
#include "CircleResizeHandle.h"
#include <Components/GraphicsObjects/MeshRenderer.h>
#include <Data/Generators/MeshConstants.h>
#include <Data/Materials/SampleDiffuse/SampleDiffuseShader.h>

namespace Jimara {
	namespace Editor {
		struct CircleResizeHandle::Helpers {
			template<typename ActionType>
			inline static void ForAllHandles(CircleResizeHandle* self, const ActionType& action) {
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
			}

			inline static void PoseShape(CircleResizeHandle* self, const Vector3& position, const Vector3& rotation, float radius) {
				self->m_poseTransform->SetLocalPosition(position);
				self->m_poseTransform->SetLocalEulerAngles(rotation);
				self->m_poseTransform->SetLocalScale(Vector3(std::abs(radius)));
			}

			inline static void PoseHandles(CircleResizeHandle* self, float radius) {
				if (self->m_poseTransform->Enabled()) {
					const constexpr float BASE_HANDLE_SIZE = 0.1f;
					const Vector3 basePosition = self->m_poseTransform->LocalPosition();
					const Vector3 rotation = self->m_poseTransform->LocalEulerAngles();
					ForAllHandles(self, [&](DragHandle* handle, Vector3 localDirection) {
						handle->SetEnabled(true);
						handle->SetLocalEulerAngles(rotation);
						const Vector3 worldDirection = handle->LocalToParentSpaceDirection(localDirection);
						const Vector3 position = basePosition + (worldDirection * radius);
						handle->SetLocalPosition(position);
						const float scaleMultiplier = handle->GizmoContext()->Viewport()->GizmoSizeAt(position);
						handle->SetLocalScale(Vector3(BASE_HANDLE_SIZE * scaleMultiplier));
						});
				}
				else ForAllHandles(self, [&](DragHandle* handle, const auto&) { handle->SetEnabled(false); });
			};

			inline static void DragHandles(CircleResizeHandle* self, float& radius) {
				if (!self->m_poseTransform->Enabled()) return;
				ForAllHandles(self, [&](DragHandle* handle, const Vector3& localDirection) {
					if (!handle->HandleActive()) return;
					const Vector3 invScale = [&]() {
						auto inverseScale = [&](float a, float b) {
							return a * ((std::abs(b) > std::numeric_limits<float>::epsilon()) ? (1.0f / b) : 0.0f);
						};
						const Vector3 totalScale = self->m_poseTransform->LocalScale();
						return Vector3(inverseScale(radius, totalScale.x), inverseScale(radius, totalScale.y), inverseScale(radius, totalScale.z));
					}();
					const Vector3 worldDelta = handle->Delta();
					const Vector3 worldDirection = handle->LocalToParentSpaceDirection(localDirection);
					const float deltaAmount = Math::Dot(worldDirection, worldDelta);
					const float scaledAmount = deltaAmount * Math::Dot(localDirection, invScale);
					radius += scaledAmount * Math::Dot(localDirection, Vector3(1.0f)) * ((radius >= 0.0f) ? 1.0f : -1.0f);
					});
			};

			inline static void InitializeRenderers(CircleResizeHandle* self) {
				const Reference<TriMesh> shape = MeshConstants::Tri::Cube();
				ForAllHandles(self, [&](DragHandle* handle, const Vector3& localDirection) {
					const Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(handle, "CircleResizeHandle_HendleRenderer", shape);
					renderer->SetMaterialInstance(self->m_poseTransform->GetComponentInChildren<MeshRenderer>()->MaterialInstance());
					renderer->SetLayer(static_cast<Layer>(GizmoLayers::HANDLE));
					});
			}

			inline static void TrackTargetsOnDragEnd(CircleResizeHandle* self, Handle*) {
				Gizmo* gizmo = self->GetComponentInParents<Gizmo>();
				if (gizmo != nullptr) gizmo->TrackTargets();
			}

			class HandleTarget : public virtual Component {
			public:
				inline HandleTarget(Scene::LogicContext* context) : Component(context, "CircleResizeHandle_HandleRoot") {}
				inline virtual ~HandleTarget() {}
			};

			template<typename Action>
			inline static void ForHandleRoot(CircleResizeHandle* self, const Action& action) {
				if (self->m_poseTransform->Parent() != nullptr && (!self->m_poseTransform->Parent()->Destroyed()))
					action(self->m_poseTransform->Parent());
			}
		};

		CircleResizeHandle::CircleResizeHandle(Component* parent, const Vector3& color)
			: Component(parent, "CircleResizeHandle")
			, m_poseTransform(Object::Instantiate<Transform>(this, "CircleResizeHandle_Pose"))
			, m_resizeRight(Object::Instantiate<DragHandle>(this, "CircleResizeHandle_ResizeRight", DragHandle::Flags::DRAG_X))
			, m_resizeLeft(Object::Instantiate<DragHandle>(this, "CircleResizeHandle_ResizeLeft", DragHandle::Flags::DRAG_X))
			, m_resizeUp(Object::Instantiate<DragHandle>(this, "CircleResizeHandle_ResizeUp", DragHandle::Flags::DRAG_Y))
			, m_resizeDown(Object::Instantiate<DragHandle>(this, "CircleResizeHandle_ResizeDown", DragHandle::Flags::DRAG_Y)) {

			Reference<Helpers::HandleTarget> parentObject = Object::Instantiate<Helpers::HandleTarget>(Context());
			m_poseTransform->SetParent(parentObject);
			Helpers::ForAllHandles(this, [&](DragHandle* handle, const Vector3&) { handle->SetParent(parentObject); });

			const Reference<TriMesh> shape = MeshConstants::Tri::WireCircle();
			const Reference<Material::Instance> material = SampleDiffuseShader::MaterialInstance(Context()->Graphics()->Device(), color);
			const Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(m_poseTransform, "CircleResizeHandle_ShapeRenderer", shape);
			renderer->SetMaterialInstance(material);
			renderer->SetLayer(static_cast<Layer>(GizmoLayers::OVERLAY));
			renderer->SetGeometryType(Graphics::Experimental::GraphicsPipeline::IndexType::EDGE);
			Helpers::InitializeRenderers(this);

			Helpers::ForAllHandles(this, [&](DragHandle* handle, const Vector3&) {
				handle->OnHandleDeactivated() += Callback(Helpers::TrackTargetsOnDragEnd, this);
				});
		}

		CircleResizeHandle::~CircleResizeHandle() {}

		void CircleResizeHandle::Update(Vector3 position, Vector3 rotation, float& radius) {
			Helpers::PoseShape(this, position, rotation, radius);
			Helpers::PoseHandles(this, radius);
			Helpers::DragHandles(this, radius);
		}

		void CircleResizeHandle::OnComponentDisabled() {
			m_poseTransform->SetEnabled(false);
			Helpers::ForAllHandles(this, [&](DragHandle* handle, const Vector3&) { handle->SetEnabled(false); });
		}

		void CircleResizeHandle::OnComponentEnabled() {
			m_poseTransform->SetEnabled(true);
			Helpers::ForAllHandles(this, [&](DragHandle* handle, const Vector3&) { handle->SetEnabled(true); });
		}

		void CircleResizeHandle::OnComponentDestroyed() {
			Helpers::ForAllHandles(this, [&](DragHandle* handle, const Vector3&) {
				handle->OnHandleDeactivated() -= Callback(Helpers::TrackTargetsOnDragEnd, this);
				});
			Helpers::ForHandleRoot(this, [&](Component* root) { root->Destroy(); });
		}
	}
}

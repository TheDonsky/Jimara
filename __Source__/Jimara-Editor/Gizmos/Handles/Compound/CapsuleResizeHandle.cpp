#include "CapsuleResizeHandle.h"
#include <Data/Generators/MeshConstants.h>
#include <Data/Materials/SampleDiffuse/SampleDiffuseShader.h>

namespace Jimara {
	namespace Editor {
		struct CapsuleResizeHandle::Helpers {
			inline static void InitializeHandle(DragHandle* handle, const Material::Instance* material) {
				const Reference<TriMesh> shape = MeshConstants::Tri::Cube();
				const Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(handle, "CapsuleResizeHandle_HandleRenderer", shape);
				renderer->SetMaterialInstance(material);
				renderer->SetLayer(static_cast<Layer>(GizmoLayers::HANDLE));
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
			static void ForAllHandles(CapsuleResizeHandle* self, const Action& action) {
				ForAllHandles(self->m_heightHandles, action);
				ForAllHandles(self->m_topRadiusHandles, action);
				ForAllHandles(self->m_bottomRadiusHandles, action);
			}

			inline static void UpdateRenderer(CapsuleResizeHandle* self, Vector3 position, Vector3 rotation, float radius, float height) {
				float scale;
				
				// Update mesh:
				{
					if (std::abs(radius) > std::numeric_limits<float>::epsilon()) {
						scale = std::abs(radius);
						height /= radius;
						radius = 1.0f;
					}
					else scale = 1.0f;
					if (self->m_lastRadius != radius || self->m_lastHeight != height) {
						Reference<TriMesh> mesh = MeshConstants::Tri::WireCapsule(radius, height);
						self->m_renderer->SetMesh(mesh);
						self->m_lastRadius = radius;
						self->m_lastHeight = height;
					}
				}

				// Update transform:
				{
					Transform* poseTransform = self->m_renderer->GetTransfrom();
					poseTransform->SetLocalPosition(position);
					poseTransform->SetLocalEulerAngles(rotation);
					poseTransform->SetLocalScale(Vector3(scale));
				}
			}

			inline static void PoseHandles(CapsuleResizeHandle* self, float radius, float height) {
				const Transform* const poseTransform = self->m_renderer->GetTransfrom();
				if (poseTransform != nullptr && self->m_renderer->Enabled()) {
					const constexpr float BASE_HANDLE_SIZE = 0.1f;
					const Vector3 basePosition = poseTransform->LocalPosition();
					const Vector3 rotation = poseTransform->LocalEulerAngles();

					// Pose radial handles:
					{
						auto poseRadiusHandles = [&](DragHandle* handle, Vector3 localDirection, float direction) {
							handle->SetEnabled(true);
							handle->SetLocalEulerAngles(rotation);
							const Vector3 worldDirection = handle->LocalToParentSpaceDirection(localDirection);
							const Vector3 position = basePosition +
								(handle->Up() * (direction * height * 0.5f)) +
								(worldDirection * radius);
							handle->SetLocalPosition(position);
							const float scaleMultiplier = handle->GizmoContext()->Viewport()->GizmoSizeAt(position);
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
							const Vector3 worldDirection = handle->LocalToParentSpaceDirection(localDirection);
							const Vector3 position = basePosition + (worldDirection * ((height * 0.5f) + radius));
							handle->SetLocalPosition(position);
							const float scaleMultiplier = handle->GizmoContext()->Viewport()->GizmoSizeAt(position);
							handle->SetLocalScale(Vector3(BASE_HANDLE_SIZE * scaleMultiplier));
						};
						ForAllHandles(self->m_heightHandles, poseHeightHandle);
					}
				}
				else ForAllHandles(self, [&](DragHandle* handle, const auto&) { handle->SetEnabled(false); });
			}

			inline static void DragHandles(CapsuleResizeHandle* self, float& radius, float& height) {
				const Transform* const poseTransform = self->m_renderer->GetTransfrom();
				if (poseTransform == nullptr || (!self->m_renderer->Enabled())) return;
				
				auto dragHandle = [&](DragHandle* handle, const Vector3& localDirection) {
					if (!handle->HandleActive()) return 0.0f;
					const Vector3 invScale = [&]() {
						auto inverseScale = [&](float a, float b) {
							return a * ((std::abs(b) > std::numeric_limits<float>::epsilon()) ? (1.0f / b) : 0.0f);
						};
						const Vector3 totalScale = poseTransform->LocalScale();
						return Vector3(inverseScale(radius, totalScale.x), inverseScale(radius, totalScale.y), inverseScale(radius, totalScale.z));
					}();
					const Vector3 worldDelta = handle->Delta();
					const Vector3 worldDirection = handle->LocalToParentSpaceDirection(localDirection);
					const float deltaAmount = Math::Dot(worldDirection, worldDelta);
					const float scaledAmount = deltaAmount * Math::Dot(localDirection, invScale);
					return scaledAmount * Math::Dot(localDirection, Vector3(1.0f)) * ((radius >= 0.0f) ? 1.0f : -1.0f);
				};

				// Drag radius:
				{
					auto dragRadiusHandle = [&](DragHandle* handle, const Vector3& localDirection) {
						radius += dragHandle(handle, localDirection);
					};
					ForAllHandles(self->m_topRadiusHandles, dragRadiusHandle);
					ForAllHandles(self->m_bottomRadiusHandles, dragRadiusHandle);
				}

				// Drag height:
				{
					auto dragHeightHandle = [&](DragHandle* handle, const Vector3& localDirection) {
						height += dragHandle(handle, localDirection) * 2.0f;
					};
					ForAllHandles(self->m_heightHandles, dragHeightHandle);
				}
			}

			inline static void TrackTargetsOnDragEnd(CapsuleResizeHandle* self, Handle*) {
				Gizmo* gizmo = self->GetComponentInParents<Gizmo>();
				if (gizmo != nullptr) gizmo->TrackTargets();
			}

			class HandleTarget : public virtual Component {
			public:
				inline HandleTarget(Scene::LogicContext* context) : Component(context, "CapsuleResizeHandle_HandleRoot") {}
				inline virtual ~HandleTarget() {}
			};

			template<typename Action>
			inline static void ForHandleRoot(CapsuleResizeHandle* self, const Action& action) {
				if (self->m_renderer->Parent() != nullptr &&
					self->m_renderer->Parent()->Parent() != nullptr &&
					(!self->m_renderer->Parent()->Parent()->Destroyed()))
					action(self->m_renderer->Parent()->Parent());
			}
		};

		CapsuleResizeHandle::RadiusHandles::RadiusHandles(CapsuleResizeHandle* parent)
			: right(Object::Instantiate<DragHandle>(parent, "CapsuleResizeHandle_RadiusHandle_Right", DragHandle::Flags::DRAG_X))
			, left(Object::Instantiate<DragHandle>(parent, "CapsuleResizeHandle_RadiusHandle_Left", DragHandle::Flags::DRAG_X))
			, front(Object::Instantiate<DragHandle>(parent, "CapsuleResizeHandle_RadiusHandle_Front", DragHandle::Flags::DRAG_Z))
			, back(Object::Instantiate<DragHandle>(parent, "CapsuleResizeHandle_RadiusHandle_Back", DragHandle::Flags::DRAG_Z)) {}

		CapsuleResizeHandle::HeightHandles::HeightHandles(CapsuleResizeHandle* parent)
			: top(Object::Instantiate<DragHandle>(parent, "CapsuleResizeHandle_HeightHandle_Top", DragHandle::Flags::DRAG_Y))
			, bottom(Object::Instantiate<DragHandle>(parent, "CapsuleResizeHandle_HeightHandle_Bottom", DragHandle::Flags::DRAG_Y)) {}

		CapsuleResizeHandle::CapsuleResizeHandle(Component* parent, const Vector3& color)
			: Component(parent, "CapsuleResizeHandle")
			, m_renderer(Object::Instantiate<MeshRenderer>(
				Object::Instantiate<Transform>(this, "CapsuleResizeHandle_Pose"), "CapsuleResizeHandle_ShapeRenderer"))
			, m_heightHandles(this)
			, m_topRadiusHandles(this)
			, m_bottomRadiusHandles(this) {

			Reference<Helpers::HandleTarget> parentObject = Object::Instantiate<Helpers::HandleTarget>(Context());
			m_renderer->Parent()->SetParent(parentObject);
			Helpers::ForAllHandles(this, [&](DragHandle* handle, const Vector3&) { handle->SetParent(parentObject); });

			const Reference<Material::Instance> material = SampleDiffuseShader::MaterialInstance(Context()->Graphics()->Device(), color);
			m_renderer->SetMaterialInstance(material);
			m_renderer->SetLayer(static_cast<Layer>(GizmoLayers::OVERLAY));
			m_renderer->SetGeometryType(Graphics::Experimental::GraphicsPipeline::IndexType::EDGE);
			Helpers::ForAllHandles(this, [&](DragHandle* handle, const Vector3&) {
				Helpers::InitializeHandle(handle, material);
				});

			Helpers::ForAllHandles(this, [&](DragHandle* handle, const Vector3&) {
				handle->OnHandleDeactivated() += Callback(Helpers::TrackTargetsOnDragEnd, this);
				});
		}
		CapsuleResizeHandle::~CapsuleResizeHandle() {}

		void CapsuleResizeHandle::Update(Vector3 position, Vector3 rotation, float& radius, float& height) {
			Helpers::UpdateRenderer(this, position, rotation, radius, height);
			Helpers::PoseHandles(this, radius, height);
			Helpers::DragHandles(this, radius, height);
		}

		void CapsuleResizeHandle::OnComponentDisabled() {
			m_renderer->SetEnabled(false);
			Helpers::ForAllHandles(this, [&](DragHandle* handle, const Vector3&) { handle->SetEnabled(false); });
		}

		void CapsuleResizeHandle::OnComponentEnabled() {
			m_renderer->SetEnabled(true);
			Helpers::ForAllHandles(this, [&](DragHandle* handle, const Vector3&) { handle->SetEnabled(true); });
		}

		void CapsuleResizeHandle::OnComponentDestroyed() {
			Helpers::ForAllHandles(this, [&](DragHandle* handle, const Vector3&) {
				handle->OnHandleDeactivated() -= Callback(Helpers::TrackTargetsOnDragEnd, this);
				});
			Helpers::ForHandleRoot(this, [&](Component* root) { root->Destroy(); });
		}
	}
}
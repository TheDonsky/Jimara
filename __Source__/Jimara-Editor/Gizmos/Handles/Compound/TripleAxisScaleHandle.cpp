#include "TripleAxisScalehandle.h"
#include <Jimara/Components/GraphicsObjects/MeshRenderer.h>
#include <Jimara/Data/Materials/SampleDiffuse/SampleDiffuseShader.h>
#include <Jimara/Data/Geometry/MeshGenerator.h>


namespace Jimara {
	namespace Editor {
		struct TripleAxisScalehandle::Helpers {
			static const constexpr float SHAPE_SIZE = 0.15f;
			static const constexpr float ARROW_SIZE = 1.0f;

			inline static TriMesh* Shape() {
				static const Reference<TriMesh> shape = GenerateMesh::Tri::Box(-Vector3(SHAPE_SIZE * 0.5f), Vector3(SHAPE_SIZE * 0.5f));
				return shape;
			}

			inline static void Initialize(const ScaleHandle& handle, Vector3 color) {
				auto materialInstance = SampleDiffuseShader::MaterialInstance(handle.handle->Context()->Graphics()->Device(), color);
				if (handle.handleConnector != nullptr) {
					handle.handleConnector->SetParent(handle.handle);
					Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(handle.handleConnector, "Renderer", Shape());
					renderer->SetMaterialInstance(materialInstance);
					renderer->SetLayer(static_cast<Layer>(GizmoLayers::HANDLE));
				}
				{
					Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(handle.handle, "Renderer", Shape());
					renderer->SetMaterialInstance(materialInstance);
					renderer->SetLayer(static_cast<Layer>(GizmoLayers::HANDLE));
				}
				Helpers::SetLocalPosition(handle, handle.defaultPosition);
			};

			inline static void SetWorldPosition(const ScaleHandle& handle, Vector3 position) {
				handle.handle->SetWorldPosition(position);
				if (handle.handleConnector != nullptr) {
					handle.handleConnector->SetLocalPosition(-handle.handle->LocalPosition() * 0.5f);
					Vector3 rawScale = -handle.handleConnector->LocalPosition() / (SHAPE_SIZE * 0.5f) + 0.25f;
					handle.handleConnector->SetLocalScale(Vector3(std::abs(rawScale.x), std::abs(rawScale.y), std::abs(rawScale.z)));
				}
				else {
					const float baseSqrSize = Math::SqrMagnitude(handle.defaultPosition);
					if (baseSqrSize > 0.0001f) {
						Vector3 scale = handle.defaultPosition * (2.0f / SHAPE_SIZE);
						scale.x = max(std::abs(scale.x), 0.1f);
						scale.y = max(std::abs(scale.y), 0.1f);
						scale.z = max(std::abs(scale.z), 0.1f);
						handle.handle->SetLocalScale(scale);
					}
				}
			}

			inline static void SetLocalPosition(const ScaleHandle& handle, Vector3 position) {
				SetWorldPosition(handle, handle.handle->GetComponentInParents<Transform>(false)->LocalToWorldPosition(position));
			}

			template<typename CallType>
			inline static bool ForAllHandles(const TripleAxisScalehandle* self, const CallType& call) {
				if (call(self->m_center)) return true;
				else if (call(self->m_xHandle)) return true;
				else if (call(self->m_yHandle)) return true;
				else if (call(self->m_zHandle)) return true;
				else if (call(self->m_xyHandle)) return true;
				else if (call(self->m_xzHandle)) return true;
				else if (call(self->m_yzHandle)) return true;
				else return false;
			}
		};

		TripleAxisScalehandle::TripleAxisScalehandle(Component* parent, const std::string_view& name, float size)
			: Component(parent, name), Transform(parent, name)
			, m_center(ScaleHandle{ FreeMoveSphereHandle(this, Vector4(1.0f), "XYZ"), Vector3(0.0f), nullptr })
			, m_xHandle(ScaleHandle{
			Object::Instantiate<DragHandle>(this, "X", DragHandle::Flags::DRAG_X),
			Vector3(Helpers::ARROW_SIZE, 0.0f, 0.0f), Object::Instantiate<Transform>(this, "X_Connector") })
			, m_yHandle(ScaleHandle{
			Object::Instantiate<DragHandle>(this, "Y", DragHandle::Flags::DRAG_Y),
			Vector3(0.0f, Helpers::ARROW_SIZE, 0.0f), Object::Instantiate<Transform>(this, "Y_Connector") })
			, m_zHandle(ScaleHandle{
			Object::Instantiate<DragHandle>(this, "Z", DragHandle::Flags::DRAG_Z),
			Vector3(0.0f, 0.0f, Helpers::ARROW_SIZE), Object::Instantiate<Transform>(this, "Z_Connector") })
			, m_xyHandle(ScaleHandle{
			Object::Instantiate<DragHandle>(this, "XY", DragHandle::Flags::DRAG_XY),
			Vector3(Helpers::SHAPE_SIZE, Helpers::SHAPE_SIZE, 0.0f), nullptr })
			, m_xzHandle(ScaleHandle{
			Object::Instantiate<DragHandle>(this, "XZ", DragHandle::Flags::DRAG_XZ),
			Vector3(Helpers::SHAPE_SIZE, 0.0f, Helpers::SHAPE_SIZE), nullptr })
			, m_yzHandle(ScaleHandle{
			Object::Instantiate<DragHandle>(this, "YZ", DragHandle::Flags::DRAG_YZ),
			Vector3(0.0f, Helpers::SHAPE_SIZE, Helpers::SHAPE_SIZE), nullptr })
			, m_hover(GizmoViewportHover::GetFor(GizmoScene::GetContext(parent->Context())->Viewport()))
			, m_size(size) {

			Helpers::Initialize(m_xHandle, Vector3(1.0f, 0.0f, 0.0f));
			Helpers::Initialize(m_yHandle, Vector3(0.0f, 1.0f, 0.0f));
			Helpers::Initialize(m_zHandle, Vector3(0.0f, 0.0f, 1.0f));

			Helpers::Initialize(m_xyHandle, Vector3(0.0f, 0.0f, 1.0f));
			Helpers::Initialize(m_xzHandle, Vector3(0.0f, 1.0f, 0.0f));
			Helpers::Initialize(m_yzHandle, Vector3(1.0f, 0.0f, 0.0f));

			Helpers::ForAllHandles(this, [&](const ScaleHandle& handle) {
				handle.handle->OnHandleActivated() += Callback(&TripleAxisScalehandle::HandleActivated, this);
				handle.handle->OnHandleUpdated() += Callback(&TripleAxisScalehandle::HandleUpdated, this);
				handle.handle->OnHandleDeactivated() += Callback(&TripleAxisScalehandle::HandleDisabled, this);
				return false;
				});
			HandleDisabled(nullptr);
			UpdateScale();
		}

		TripleAxisScalehandle::~TripleAxisScalehandle() {}

		bool TripleAxisScalehandle::HandleActive()const {
			return Helpers::ForAllHandles(this, [](const ScaleHandle& handle) { return handle.handle->HandleActive(); });
		}

		void TripleAxisScalehandle::Update() { UpdateScale(); }

		void TripleAxisScalehandle::OnComponentDestroyed() {
			Transform::OnComponentDestroyed();
			Scene::LogicContext::UpdatingComponent::OnComponentDestroyed();
			Helpers::ForAllHandles(this, [&](const ScaleHandle& handle) {
				handle.handle->OnHandleActivated() -= Callback(&TripleAxisScalehandle::HandleActivated, this);
				handle.handle->OnHandleUpdated() -= Callback(&TripleAxisScalehandle::HandleUpdated, this);
				handle.handle->OnHandleDeactivated() -= Callback(&TripleAxisScalehandle::HandleDisabled, this);
				return false;
				});
		}


		void TripleAxisScalehandle::HandleActivated(Handle*) {
			m_hoverOrigin = m_hover->HandleGizmoHover().objectPosition;
			m_delta = Vector3(0.0f);
			m_scale = Vector3(1.0f);
			m_onHandleActivated(this);
		}

		void TripleAxisScalehandle::HandleUpdated(Handle* handle) {
			const ScaleHandle* scaleHandle = nullptr;
			Helpers::ForAllHandles(this, [&](const ScaleHandle& ref) {
				if (ref.handle == handle) {
					scaleHandle = &ref;
					return true;
				}
				else return false;
				});
			if (scaleHandle == nullptr) {
				Context()->Log()->Error("TripleAxisScalehandle::HandleUpdated - Got input from unknown handle!");
				return;
			}

			Vector3 rawDelta = dynamic_cast<DragHandle*>(handle)->Delta();
			if (scaleHandle == &m_center) {
				const Vector3 curScale = scaleHandle->handle->LocalScale();
				const Vector3 initialDelta = m_hoverOrigin - WorldPosition();
				float deltaScale = Math::Dot(rawDelta, initialDelta) / Math::SqrMagnitude(initialDelta);
				if (!std::isfinite(deltaScale)) deltaScale = 0.0f;
				const Vector3 finalScale = curScale + deltaScale;
				scaleHandle->handle->SetLocalScale(finalScale);
				m_delta = Vector3(deltaScale);
				m_scale = finalScale;
			}
			else {
				const float baseDelta = Math::Magnitude(scaleHandle->defaultPosition);
				auto scaleFactor = [&]() {
					return Math::Magnitude(scaleHandle->handle->LocalPosition()) / baseDelta *
						(Math::Dot(scaleHandle->handle->LocalPosition(), scaleHandle->defaultPosition) >= 0.0f ? 1.0f : -1.0f);
				};
				float curVal = scaleFactor();
				{
					const Vector3 scaleDirection = Math::Normalize(LocalToWorldDirection(scaleHandle->defaultPosition));
					Helpers::SetWorldPosition(*scaleHandle,
						scaleHandle->handle->WorldPosition() +
						(scaleDirection * Math::Dot(rawDelta, scaleDirection)));
				}
				float newValue = scaleFactor();
				const Vector3 defaultDirection = Math::Normalize(scaleHandle->defaultPosition);
				m_delta = (defaultDirection * (newValue - curVal));
				m_scale += m_delta;
			}
			m_onHandleUpdated(this);
		}

		void TripleAxisScalehandle::HandleDisabled(Handle*) {
			m_delta = Vector3(0.0f);
			m_scale = Vector3(1.0f);
			Helpers::ForAllHandles(this, [](const ScaleHandle& handle) {
				Helpers::SetLocalPosition(handle, handle.defaultPosition);
				return false;
				});
			m_center.handle->SetLocalScale(Vector3(1.0f));
			m_onHandleDeactivated(this);
		}

		void TripleAxisScalehandle::UpdateScale() {
			SetLocalScale(Vector3(m_size * m_center.handle->GizmoContext()->Viewport()->GizmoSizeAt(WorldPosition())));
		}
	}
}

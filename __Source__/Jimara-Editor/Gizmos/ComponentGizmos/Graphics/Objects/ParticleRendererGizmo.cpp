#include "ParticleRendererGizmo.h"
#include <Components/GraphicsObjects/ParticleRenderer.h>
#include <Data/Geometry/MeshConstants.h>


namespace Jimara {
	namespace Editor {
		ParticleRendererGizmo::ParticleRendererGizmo(Scene::LogicContext* context)
			: Component(context, "ParticleRendererGizmo")
			, m_handle(Object::Instantiate<Transform>(this, "ParticleRendererGizmo_Handle")) {
			m_handle->SetEnabled(false);
			const Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(m_handle);
			renderer->SetLayer(static_cast<Layer>(GizmoLayers::WORLD_SPACE));
			renderer->SetGeometryType(Graphics::GraphicsPipeline::IndexType::EDGE);
			renderer->SetMesh(MeshConstants::Tri::WireCube());
		}

		ParticleRendererGizmo::~ParticleRendererGizmo() {}

		void ParticleRendererGizmo::Update() {
			Reference<ParticleRenderer> renderer = Target<ParticleRenderer>();
			if (renderer == nullptr || (!renderer->ActiveInHeirarchy())) {
				m_handle->SetEnabled(false);
				return;
			}

			const AABB bounds = renderer->GetLocalBoundaries();
			if (std::isnan(bounds.start.x) || std::isnan(bounds.end.x) ||
				std::isnan(bounds.start.y) || std::isnan(bounds.end.y) ||
				std::isnan(bounds.start.z) || std::isnan(bounds.end.z)) {
				m_handle->SetEnabled(false);
				return;
			}

			const Transform* rendererTransform = renderer->GetTransfrom();
			if (rendererTransform != nullptr) {
				m_handle->SetLocalPosition(rendererTransform->LocalToWorldPosition((bounds.start + bounds.end) * 0.5f));
				m_handle->SetLocalEulerAngles(rendererTransform->WorldEulerAngles());
				m_handle->SetLocalScale(rendererTransform->LossyScale() * (bounds.end - bounds.start));
			}
			else {
				m_handle->SetLocalPosition((bounds.start + bounds.end) * 0.5f);
				m_handle->SetLocalEulerAngles(Vector3(0.0f));
				m_handle->SetLocalScale(bounds.end - bounds.start);
			}
			m_handle->SetEnabled(true);
		}

		namespace {
			static const constexpr Gizmo::ComponentConnection ParticleRendererGizmo_Connection =
				Gizmo::ComponentConnection::Make<ParticleRendererGizmo, ParticleRenderer>();
		}
	}


	template<> void TypeIdDetails::OnRegisterType<Editor::ParticleRendererGizmo>() {
		Editor::Gizmo::AddConnection(Editor::ParticleRendererGizmo_Connection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::ParticleRendererGizmo>() {
		Editor::Gizmo::RemoveConnection(Editor::ParticleRendererGizmo_Connection);
	}
}

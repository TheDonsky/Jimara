#include "ParticleRendererGizmo.h"
#include <Jimara/Components/GraphicsObjects/ParticleRenderer.h>
#include <Jimara/Components/GraphicsObjects/MeshRenderer.h>
#include <Jimara/Data/Geometry/MeshConstants.h>


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
			if (renderer == nullptr || (!renderer->ActiveInHierarchy())) {
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

			const Transform* rendererTransform = renderer->GetTransform();
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
	}


	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::ParticleRendererGizmo>(const Callback<const Object*>& report) {
		static const Reference<const Editor::Gizmo::ComponentConnection> connection = 
			Editor::Gizmo::ComponentConnection::Make<Editor::ParticleRendererGizmo, ParticleRenderer>();
		report(connection);
	}
}

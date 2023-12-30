#include "SubsceneGizmo.h"
#include <Components/Level/Subscene.h>
#include <Components/GraphicsObjects/MeshRenderer.h>
#include <Data/Geometry/MeshConstants.h>


namespace Jimara {
	namespace Editor {
		SubsceneGizmo::SubsceneGizmo(Scene::LogicContext* context)
			: Component(context, "SubsceneGizmo")
			, m_handle(Object::Instantiate<Transform>(this, "SubsceneGizmo_Handle")) {
			const Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(m_handle);
			renderer->SetLayer(static_cast<Layer>(GizmoLayers::WORLD_SPACE));
			renderer->SetGeometryType(Graphics::GraphicsPipeline::IndexType::EDGE);
			renderer->SetMesh(MeshConstants::Tri::WireCube());
		}

		SubsceneGizmo::~SubsceneGizmo() {}

		void SubsceneGizmo::Update() {
			Reference<Subscene> subscene = Target<Subscene>();
			if (subscene == nullptr || (!subscene->ActiveInHeirarchy())) {
				m_handle->SetEnabled(false);
				return;
			}
			
			const AABB bounds = subscene->GetBoundaries();
			if (std::isnan(bounds.start.x) || std::isnan(bounds.end.x) ||
				std::isnan(bounds.start.y) || std::isnan(bounds.end.y) ||
				std::isnan(bounds.start.z) || std::isnan(bounds.end.z)) {
				m_handle->SetEnabled(false);
				return;
			}

			m_handle->SetLocalPosition((bounds.start + bounds.end) * 0.5f);
			m_handle->SetLocalScale(bounds.end - bounds.start);
			m_handle->SetEnabled(true);
		}

		namespace {
			static const constexpr Gizmo::ComponentConnection SubsceneGizmo_Connection =
				Gizmo::ComponentConnection::Make<SubsceneGizmo, Subscene>();
		}
	}


	template<> void TypeIdDetails::OnRegisterType<Editor::SubsceneGizmo>() {
		Editor::Gizmo::AddConnection(Editor::SubsceneGizmo_Connection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::SubsceneGizmo>() {
		Editor::Gizmo::RemoveConnection(Editor::SubsceneGizmo_Connection);
	}
}

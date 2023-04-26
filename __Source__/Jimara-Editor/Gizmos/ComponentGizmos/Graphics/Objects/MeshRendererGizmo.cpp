#include "MeshRendererGizmo.h"



namespace Jimara {
	namespace Editor {
		MeshRendererGizmo::MeshRendererGizmo(Scene::LogicContext* context) 
			: Component(context, "MeshRendererGizmo")
			, m_wireframeRenderer(
				Object::Instantiate<MeshRenderer>(
					Object::Instantiate<Transform>(this, "MeshRendererGizmo_Transform"), 
					"MeshRendererGizmo_Renderer")) {
			m_wireframeRenderer->SetLayer(static_cast<Layer>(GizmoLayers::WORLD_SPACE));
			m_wireframeRenderer->SetGeometryType(Graphics::GraphicsPipeline::IndexType::EDGE);
		}

		MeshRendererGizmo::~MeshRendererGizmo() {}

		void MeshRendererGizmo::Update() {
			const MeshRenderer* const target = Target<MeshRenderer>();
			if (target == nullptr) {
				m_wireframeRenderer->SetMesh(nullptr);
				return;
			}
			const Transform* const targetTransform = target->GetTransfrom();
			{
				m_wireframeRenderer->SetEnabled(target->ActiveInHeirarchy() && (targetTransform != nullptr));
				m_wireframeRenderer->SetMesh(target->Mesh());
			}
			if (targetTransform != nullptr) {
				Transform* const wireTransform = m_wireframeRenderer->GetTransfrom();
				wireTransform->SetLocalPosition(targetTransform->WorldPosition());
				wireTransform->SetLocalEulerAngles(targetTransform->WorldEulerAngles());
				wireTransform->SetLocalScale(targetTransform->LossyScale());
			}
		}

		namespace {
			static const constexpr Gizmo::ComponentConnection MeshRendererGizmo_Connection =
				Gizmo::ComponentConnection::Make<MeshRendererGizmo, MeshRenderer>();
		}
	}


	template<> void TypeIdDetails::OnRegisterType<Editor::MeshRendererGizmo>() {
		Editor::Gizmo::AddConnection(Editor::MeshRendererGizmo_Connection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::MeshRendererGizmo>() {
		Editor::Gizmo::RemoveConnection(Editor::MeshRendererGizmo_Connection);
	}
}


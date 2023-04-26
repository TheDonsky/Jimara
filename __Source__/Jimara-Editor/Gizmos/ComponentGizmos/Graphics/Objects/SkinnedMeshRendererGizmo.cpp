#include "SkinnedMeshRendererGizmo.h"



namespace Jimara {
	namespace Editor {
		SkinnedMeshRendererGizmo::SkinnedMeshRendererGizmo(Scene::LogicContext* context)
			: Component(context, "SkinnedMeshRendererGizmo")
			, m_wireframeRenderer(
				Object::Instantiate<SkinnedMeshRenderer>(
					Object::Instantiate<Transform>(this, "SkinnedMeshRendererGizmo_Transform"),
					"SkinnedMeshRendererGizmo_Renderer")) {
			m_wireframeRenderer->SetLayer(static_cast<Layer>(GizmoLayers::WORLD_SPACE));
			m_wireframeRenderer->SetGeometryType(Graphics::GraphicsPipeline::IndexType::EDGE);
		}

		SkinnedMeshRendererGizmo::~SkinnedMeshRendererGizmo() {}

		void SkinnedMeshRendererGizmo::Update() {
			const SkinnedMeshRenderer* const target = Target<SkinnedMeshRenderer>();
			if (target == nullptr) {
				m_wireframeRenderer->SetMesh(nullptr);
				return;
			}
			const Transform* const targetTransform = target->GetTransfrom();
			{
				m_wireframeRenderer->SetEnabled(target->ActiveInHeirarchy() && (targetTransform != nullptr));
				m_wireframeRenderer->SetMesh(target->Mesh());
			}
			auto copyTransform = [](const Transform* src, Transform* dst) {
				if (dst == nullptr) return;
				else if (src == nullptr) {
					dst->SetLocalPosition(Vector3(0.0f));
					dst->SetLocalEulerAngles(Vector3(0.0f));
					dst->SetLocalScale(Vector3(1.0f));
				}
				else {
					dst->SetLocalPosition(src->WorldPosition());
					dst->SetLocalEulerAngles(src->WorldEulerAngles());
					dst->SetLocalScale(src->LossyScale());
				}
			};
			copyTransform(targetTransform, m_wireframeRenderer->GetTransfrom());
			for (size_t boneId = 0; boneId < target->BoneCount(); boneId++) {
				while (boneId >= m_bones.size()) {
					Reference<Transform> bone = Object::Instantiate<Transform>(this, "Bone");
					m_wireframeRenderer->SetBone(m_bones.size(), bone);
					m_bones.push_back(bone);
				}
				copyTransform(target->Bone(boneId), m_bones[boneId]);
			}
		}

		namespace {
			static const constexpr Gizmo::ComponentConnection SkinnedMeshRendererGizmo_Connection =
				Gizmo::ComponentConnection::Make<SkinnedMeshRendererGizmo, SkinnedMeshRenderer>();
		}
	}


	template<> void TypeIdDetails::OnRegisterType<Editor::SkinnedMeshRendererGizmo>() {
		Editor::Gizmo::AddConnection(Editor::SkinnedMeshRendererGizmo_Connection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::SkinnedMeshRendererGizmo>() {
		Editor::Gizmo::RemoveConnection(Editor::SkinnedMeshRendererGizmo_Connection);
	}
}

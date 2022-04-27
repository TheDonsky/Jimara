#include "TransformGizmo.h"
#include <Data/Generators/MeshGenerator.h>
#include <Components/GraphicsObjects/MeshRenderer.h>


namespace Jimara {
	namespace Editor {
		namespace {
			static const Reference<TriMesh> tmpSphere = GenerateMesh::Tri::Sphere(Vector3(0.0f), 0.125f, 32, 16);
		}

		TransformGizmo::TransformGizmo(Scene::LogicContext* context)
			: Component(context, "TransformGizmo")
			, m_transform(Object::Instantiate<Transform>(this)) {
			Object::Instantiate<MeshRenderer>(m_transform, "TransformGizmoRenderer", tmpSphere);
		}

		TransformGizmo::~TransformGizmo() {
			std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());
			if (!m_transform->Destroyed()) m_transform->Destroy();
		}

		namespace {
			inline static bool GetTargetTransforms(TransformGizmo* gizmo, std::vector<Transform*>& targetTransforms) {
				targetTransforms.clear();
				for (size_t i = 0; i < gizmo->TargetCount(); i++) {
					Transform* target = gizmo->Target<Transform>(i);
					if (target == nullptr)
						gizmo->Context()->Log()->Error(
							"TransformGizmo::Update::GetTargetTransforms - All targets are expected to be transforms! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					else {
						Transform* parent = target->GetComponentInParents<Transform>(false);
						while (parent != nullptr) {
							if (gizmo->HasTarget(parent)) 
								break;
							else parent = parent->GetComponentInParents<Transform>(false);
						}
						if (parent == nullptr)
							targetTransforms.push_back(target);
					}
				}
				return (!targetTransforms.empty());
			}
		}

		void TransformGizmo::Update() {
			if (TargetCount() <= 0) return;
			static thread_local std::vector<Transform*> targetTransforms;
			if (GetTargetTransforms(this, targetTransforms)) {
				m_transform->SetWorldPosition([&]() {
					Vector3 centerSum = Vector3(0.0f);
					for (Transform* target : targetTransforms)
						centerSum += target->WorldPosition();
					return centerSum / static_cast<float>(targetTransforms.size());
					}());
			}
			targetTransforms.clear();
		}

		namespace {
			static const constexpr Gizmo::ComponentConnection TransformGizmo_Connection =
				Gizmo::ComponentConnection::Make<TransformGizmo, Transform>(
					Gizmo::FilterFlag::CREATE_IF_SELECTED 
					| Gizmo::FilterFlag::CREATE_IF_CHILD_SELECTED 
					| Gizmo::FilterFlag::CREATE_CHILD_GIZMOS_IF_SELECTED 
					| Gizmo::FilterFlag::CREATE_ONE_FOR_ALL_TARGETS
					);
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::TransformGizmo>() {
		Editor::Gizmo::AddConnection(Editor::TransformGizmo_Connection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::TransformGizmo>() {
		Editor::Gizmo::RemoveConnection(Editor::TransformGizmo_Connection);
	}
}

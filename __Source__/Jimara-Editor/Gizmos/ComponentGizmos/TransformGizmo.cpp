#include "TransformGizmo.h"
#include <Data/Generators/MeshGenerator.h>
#include <Components/GraphicsObjects/MeshRenderer.h>
#include "../../GUI/ImGuiRenderer.h"
#include <IconFontCppHeaders/IconsFontAwesome5.h>


namespace Jimara {
	namespace Editor {
		TransformGizmo::TransformGizmo(Scene::LogicContext* context)
			: Component(context, "TransformGizmo")
			, m_moveHandle(Object::Instantiate<TripleAxisMoveHandle>(this, "TransformGizmo_MoveHandle"))
			, m_scaleHandle(Object::Instantiate<TripleAxisScalehandle>(this, "TransformGizmo_ScaleHandle")) {
			m_scaleHandle->SetEnabled(false);
		}

		TransformGizmo::~TransformGizmo() {}

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

		void TransformGizmo::OnDrawGizmoGUI() {
			auto enableHandle = [&](Component* handle) {
				auto enableIfSame = [&](Component* component) { component->SetEnabled(component == handle); };
				enableIfSame(m_moveHandle);
				enableIfSame(m_scaleHandle);
			};
			auto toggleWithButton = [&](Component* handle, auto... buttonText) {
				const bool wasEnabled = handle->Enabled();
				if (wasEnabled) ImGui::BeginDisabled();
				if (ImGui::Button(buttonText...)) enableHandle(handle);
				if (wasEnabled) ImGui::EndDisabled();
			};
			toggleWithButton(m_moveHandle, ICON_FA_ARROWS_ALT "###transform_handles_move_mode_on");
			ImGui::SameLine();
			toggleWithButton(m_scaleHandle, ICON_FA_EXPAND "###transform_handles_scale_mode_on");
		}

		void TransformGizmo::Update() {
			if (TargetCount() <= 0) return;
			static thread_local std::vector<Transform*> targetTransforms;
			if (GetTargetTransforms(this, targetTransforms)) {
				if (m_moveHandle->HandleActive()) {
					Vector3 delta = m_moveHandle->Delta();
					for (Transform* target : targetTransforms)
						target->SetWorldPosition(target->WorldPosition() + delta);
				}
				if (m_scaleHandle->HandleActive()) {
					// __TODO__: Implement this crap!
				}
				{
					const Vector3 center = [&]() {
						Vector3 centerSum = Vector3(0.0f);
						for (Transform* target : targetTransforms)
							centerSum += target->WorldPosition();
						return centerSum / static_cast<float>(targetTransforms.size());
					}();
					m_moveHandle->SetWorldPosition(center);
					m_scaleHandle->SetWorldPosition(center);
				}
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

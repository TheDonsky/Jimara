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

			inline static void MoveTransforms(TripleAxisMoveHandle* moveHandle, const std::vector<Transform*>& targetTransforms) {
				if (!moveHandle->HandleActive()) return;
				Vector3 delta = moveHandle->Delta();
				for (Transform* target : targetTransforms)
					target->SetWorldPosition(target->WorldPosition() + delta);
			}

			inline static void ScaleTransforms(TripleAxisScalehandle* scaleHandle, const std::vector<Transform*>& targetTransforms) {
				if (!scaleHandle->HandleActive()) return;
				Vector3 delta = scaleHandle->Delta();

				const Vector3 handleX = scaleHandle->Right();
				const Vector3 handleY = scaleHandle->Up();
				const Vector3 handleZ = scaleHandle->Forward();

				for (Transform* target : targetTransforms) {
					const Vector3 targetX = target->Right();
					const Vector3 targetY = target->Up();
					const Vector3 targetZ = target->Forward();

					auto toSpace = [](const Vector3& direction, const Vector3& refX, const Vector3& refY, const Vector3& refZ) {
						return Vector3(
							Math::Dot(direction, refX),
							Math::Dot(direction, refY),
							Math::Dot(direction, refZ));
					};
					auto fromSpace = [](const Vector3& direction, const Vector3& refX, const Vector3& refY, const Vector3& refZ) {
						return (direction.x * refX) + (direction.y * refY) + (direction.z * refZ);
					};

					const Vector3 handlePoint = toSpace(targetX + targetY + targetZ, handleX, handleY, handleZ);
					const Vector3 scaledPoint = handlePoint * delta;
					const Vector3 scaleDelta = toSpace(fromSpace(scaledPoint, handleX, handleY, handleZ), targetX, targetY, targetZ);

					target->SetLocalScale(target->LocalScale() + scaleDelta);
				}
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
			if (!GetTargetTransforms(this, targetTransforms)) return;
			MoveTransforms(m_moveHandle, targetTransforms);
			ScaleTransforms(m_scaleHandle, targetTransforms);
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

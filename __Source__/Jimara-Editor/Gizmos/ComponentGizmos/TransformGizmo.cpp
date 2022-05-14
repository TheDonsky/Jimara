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
			{
				m_moveHandle->OnHandleActivated() += Callback(&TransformGizmo::OnMoveStarted, this);
				m_moveHandle->OnHandleUpdated() += Callback(&TransformGizmo::OnMove, this);
				m_moveHandle->OnHandleDeactivated() += Callback(&TransformGizmo::OnMoveEnded, this);
			}
			{
				m_scaleHandle->OnHandleActivated() += Callback(&TransformGizmo::OnScaleStarted, this);
				m_scaleHandle->OnHandleUpdated() += Callback(&TransformGizmo::OnScale, this);
				m_scaleHandle->OnHandleDeactivated() += Callback(&TransformGizmo::OnScaleEnded, this);
			}
		}

		TransformGizmo::~TransformGizmo() {}

		namespace {
			template<typename RefType>
			inline static bool GetTargetTransforms(TransformGizmo* gizmo, std::vector<RefType>& targetTransforms) {
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

			inline static bool UseSteps(TransformGizmo* gizmo) { 
				return 
					gizmo->Context()->Input()->KeyPressed(OS::Input::KeyCode::LEFT_CONTROL) ||
					gizmo->Context()->Input()->KeyPressed(OS::Input::KeyCode::RIGHT_CONTROL);
			}
			inline static float StepFloat(float v, float step) { return static_cast<float>(static_cast<int>(v / step)) * step; }
			inline static Vector3 StepVector(const Vector3& v, const Vector3& step) { 
				return Vector3(StepFloat(v.x, step.x), StepFloat(v.y, step.y), StepFloat(v.z, step.z)); 
			}

			static const constexpr float MOVE_STEP = 0.1f;
			static const constexpr float SCALE_STEP = 0.1f;
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

		void TransformGizmo::OnComponentDestroyed() {
			{
				m_moveHandle->OnHandleActivated() -= Callback(&TransformGizmo::OnMoveStarted, this);
				m_moveHandle->OnHandleUpdated() -= Callback(&TransformGizmo::OnMove, this);
				m_moveHandle->OnHandleDeactivated() -= Callback(&TransformGizmo::OnMoveEnded, this);
			}
			{
				m_scaleHandle->OnHandleActivated() -= Callback(&TransformGizmo::OnScaleStarted, this);
				m_scaleHandle->OnHandleUpdated() -= Callback(&TransformGizmo::OnScale, this);
				m_scaleHandle->OnHandleDeactivated() -= Callback(&TransformGizmo::OnScaleEnded, this);
			}
			m_targetData.clear();
		}


		TransformGizmo::TargetData::TargetData(Transform* t)
			: target(t), initialPosition(t->WorldPosition()), initialLossyScale(t->LossyScale()) {}
		void TransformGizmo::FillTargetData() {
			GetTargetTransforms(this, m_targetData);
		}

		// Move handle callbacks:
		void TransformGizmo::OnMoveStarted(TripleAxisMoveHandle*) { FillTargetData(); }
		void TransformGizmo::OnMove(TripleAxisMoveHandle*) {
			Vector3 dragAmount = m_moveHandle->DragAmount();
			Vector3 processedDelta = UseSteps(this) ? StepVector(dragAmount, Vector3(MOVE_STEP)) : dragAmount;
			for (const TargetData& data : m_targetData)
				data.target->SetWorldPosition(data.initialPosition + processedDelta);
		}
		void TransformGizmo::OnMoveEnded(TripleAxisMoveHandle*) { m_targetData.clear(); }

		// Scale handle callbacks:
		void TransformGizmo::OnScaleStarted(TripleAxisScalehandle*) { FillTargetData(); }
		void TransformGizmo::OnScale(TripleAxisScalehandle*) {
			const Vector3 scale = m_scaleHandle->Scale();
			const Vector3 processedScale = UseSteps(this) ? StepVector(scale, Vector3(SCALE_STEP)) : scale;

			const Vector3 handleX = m_scaleHandle->Right();
			const Vector3 handleY = m_scaleHandle->Up();
			const Vector3 handleZ = m_scaleHandle->Forward();

			for (const TargetData& data : m_targetData) {
				const Vector3 targetX = data.target->Right();
				const Vector3 targetY = data.target->Up();
				const Vector3 targetZ = data.target->Forward();

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
				const Vector3 scaledPoint = handlePoint * processedScale;
				const Vector3 scaleDelta = toSpace(fromSpace(scaledPoint, handleX, handleY, handleZ), targetX, targetY, targetZ);

				data.target->SetLocalScale(data.initialLossyScale * scaleDelta);
			}
		}
		void TransformGizmo::OnScaleEnded(TripleAxisScalehandle*) { m_targetData.clear(); }

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

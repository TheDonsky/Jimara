#include "TransformGizmo.h"
#include <Jimara/Data/Geometry/MeshGenerator.h>
#include <Jimara/Components/GraphicsObjects/MeshRenderer.h>


namespace Jimara {
	namespace Editor {
		TransformGizmo::TransformGizmo(Scene::LogicContext* context)
			: Component(context, "TransformGizmo")
			, m_settings(TransformHandleSettings::Of(GizmoScene::GetContext(context)))
			, m_moveHandle(Object::Instantiate<TripleAxisMoveHandle>(this, "TransformGizmo_MoveHandle"))
			, m_rotationHandle(Object::Instantiate<TripleAxisRotationHandle>(this, "TransformGizmo_RotationHandle"))
			, m_scaleHandle(Object::Instantiate<TripleAxisScalehandle>(this, "TransformGizmo_ScaleHandle")) {
			{
				m_moveHandle->SetEnabled(false);
				m_moveHandle->OnHandleActivated() += Callback(&TransformGizmo::OnMoveStarted, this);
				m_moveHandle->OnHandleUpdated() += Callback(&TransformGizmo::OnMove, this);
				m_moveHandle->OnHandleDeactivated() += Callback(&TransformGizmo::OnMoveEnded, this);
			}
			{
				m_rotationHandle->SetEnabled(false);
				m_rotationHandle->OnHandleActivated() += Callback(&TransformGizmo::OnRotationStarted, this);
				m_rotationHandle->OnHandleUpdated() += Callback(&TransformGizmo::OnRotation, this);
				m_rotationHandle->OnHandleDeactivated() += Callback(&TransformGizmo::OnRotationEnded, this);
			}
			{
				m_scaleHandle->SetEnabled(false);
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
			inline static float StepFloat(float v, float step) { 
				const float fraction = (v / step);
				const float fractionSign = (fraction >= 0.0f) ? 1.0f : (-1.0f);
				return static_cast<float>(fractionSign * static_cast<int>(fraction * fractionSign)) * step;
			}
			inline static Vector3 StepVector(const Vector3& v, const Vector3& step) { 
				return Vector3(StepFloat(v.x, step.x), StepFloat(v.y, step.y), StepFloat(v.z, step.z)); 
			}

			static const constexpr float MOVE_STEP = 0.1f;
			static const constexpr float ROTATION_STEP = 10.0f;
			static const constexpr float SCALE_STEP = 0.1f;
		}

		void TransformGizmo::Update() {
			// No need to relocate anything, if the handles are active:
			if (m_rotationHandle->HandleActive() || m_scaleHandle->HandleActive()) return;

			// Collect relevant samples:
			static thread_local std::vector<Transform*> targetTransforms;
			const bool hasTargets = GetTargetTransforms(this, targetTransforms);


			// Enable/disable handles:
			{
				auto enableOrDisableHandle = [&](Transform* handle, TransformHandleSettings::HandleType type) {
					const bool shouldEnable = (m_settings->HandleMode() == type) && hasTargets;
					if (handle->Enabled() == shouldEnable)
						return;
					handle->SetLocalScale(Vector3(0.0f));
					handle->SetEnabled(shouldEnable);
				};
				enableOrDisableHandle(m_moveHandle, TransformHandleSettings::HandleType::MOVE);
				enableOrDisableHandle(m_rotationHandle, TransformHandleSettings::HandleType::ROTATE);
				enableOrDisableHandle(m_scaleHandle, TransformHandleSettings::HandleType::SCALE);
			}

			// Eearly exit, if targetTransforms is empty
			if (!hasTargets)
				return;
			
			// Update position:
			{
				const Vector3 center = [&]() {
					Vector3 centerSum = Vector3(0.0f);
					for (Transform* target : targetTransforms)
						centerSum += target->WorldPosition();
					return centerSum / static_cast<float>(targetTransforms.size());
				}();
				m_moveHandle->SetWorldPosition(center);
				m_rotationHandle->SetWorldPosition(center);
				m_scaleHandle->SetWorldPosition(center);
			}

			// Update rotation:
			{
				const Vector3 eulerAngles =
					(m_settings->HandleOrientation() == TransformHandleSettings::AxisSpace::LOCAL 
						&& [&]()->bool { 
							if (targetTransforms.size() <= 0u) return false;
							else if (targetTransforms.size() == 1) return true;
							Vector3 angles = targetTransforms[0]->WorldEulerAngles();
							for (size_t i = 1; i < targetTransforms.size(); i++)
								if (Math::SqrMagnitude(targetTransforms[i]->WorldEulerAngles() - angles) > 0.01f) return false;
							return true;
						}())
					? targetTransforms[0]->WorldEulerAngles() : Vector3(0.0f);
				m_moveHandle->SetWorldEulerAngles(eulerAngles);
				m_rotationHandle->SetWorldEulerAngles(eulerAngles);
				m_scaleHandle->SetWorldEulerAngles(eulerAngles);
			}

			// Cleanup:
			targetTransforms.clear();
		}

		void TransformGizmo::OnComponentDestroyed() {
			{
				m_moveHandle->OnHandleActivated() -= Callback(&TransformGizmo::OnMoveStarted, this);
				m_moveHandle->OnHandleUpdated() -= Callback(&TransformGizmo::OnMove, this);
				m_moveHandle->OnHandleDeactivated() -= Callback(&TransformGizmo::OnMoveEnded, this);
			}
			{
				m_rotationHandle->OnHandleActivated() -= Callback(&TransformGizmo::OnRotationStarted, this);
				m_rotationHandle->OnHandleUpdated() -= Callback(&TransformGizmo::OnRotation, this);
				m_rotationHandle->OnHandleDeactivated() -= Callback(&TransformGizmo::OnRotationEnded, this);
			}
			{
				m_scaleHandle->OnHandleActivated() -= Callback(&TransformGizmo::OnScaleStarted, this);
				m_scaleHandle->OnHandleUpdated() -= Callback(&TransformGizmo::OnScale, this);
				m_scaleHandle->OnHandleDeactivated() -= Callback(&TransformGizmo::OnScaleEnded, this);
			}
			m_targetData.clear();
		}


		TransformGizmo::TargetData::TargetData(Transform* t)
			: target(t)
			, initialPosition(t->WorldPosition())
			, initialRotation(t->WorldRotationMatrix())
			, initialLocalScale(t->LocalScale()) {}
		void TransformGizmo::FillTargetData() {
			m_initialHandleRotation = m_rotationHandle->WorldRotationMatrix();
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

		// Rotation handle callbacks:
		void TransformGizmo::OnRotationStarted(TripleAxisRotationHandle*) { FillTargetData(); }
		void TransformGizmo::OnRotation(TripleAxisRotationHandle*) {
			const Matrix4 rotation = [&]() -> Matrix4 {
				float angle = m_rotationHandle->RotationAngle();
				if (UseSteps(this)) angle = StepFloat(angle, ROTATION_STEP);
				return Math::ToMatrix(Math::AxisAngle(m_rotationHandle->RotationAxis(), angle));
			}();

			const bool useCenter = (m_settings->PivotPosition() == TransformHandleSettings::PivotMode::AVERAGE);
			const Vector3 center = m_rotationHandle->WorldPosition();

			for (const TargetData& data : m_targetData) {
				data.target->SetWorldEulerAngles(Math::EulerAnglesFromMatrix(rotation * data.initialRotation));
				if (useCenter) data.target->SetWorldPosition(center + Vector3(rotation * Vector4(data.initialPosition - center, 0.0f)));
			}
			m_rotationHandle->SetWorldEulerAngles(Math::EulerAnglesFromMatrix(rotation * m_initialHandleRotation));
		}
		void TransformGizmo::OnRotationEnded(TripleAxisRotationHandle*) { m_targetData.clear(); }

		// Scale handle callbacks:
		void TransformGizmo::OnScaleStarted(TripleAxisScalehandle*) { FillTargetData(); }
		void TransformGizmo::OnScale(TripleAxisScalehandle*) {
			const Vector3 scale = m_scaleHandle->Scale();
			const Vector3 processedScale = UseSteps(this) ? StepVector(scale, Vector3(SCALE_STEP)) : scale;

			const Vector3 handleX = m_scaleHandle->Right();
			const Vector3 handleY = m_scaleHandle->Up();
			const Vector3 handleZ = m_scaleHandle->Forward();

			const bool useCenter = (m_settings->PivotPosition() == TransformHandleSettings::PivotMode::AVERAGE);
			const Vector3 center = m_scaleHandle->WorldPosition();

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
				const Vector3 targetScale = toSpace(fromSpace(scaledPoint, handleX, handleY, handleZ), targetX, targetY, targetZ);

				data.target->SetLocalScale(data.initialLocalScale * targetScale);
				if (useCenter) {
					const Vector3 relativeOffset = toSpace(data.initialPosition - center, handleX, handleY, handleZ);
					const Vector3 scaledOffset = fromSpace(relativeOffset * processedScale, handleX, handleY, handleZ);
					data.target->SetWorldPosition(center + scaledOffset);
				}
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

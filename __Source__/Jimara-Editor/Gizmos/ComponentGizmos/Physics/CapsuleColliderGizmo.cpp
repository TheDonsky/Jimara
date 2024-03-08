#include "CapsuleColliderGizmo.h"
#include <Jimara/Components/Physics/CapsuleCollider.h>
#include <Jimara/Data/Geometry/MeshConstants.h>
#include <Jimara/Data/Materials/SampleDiffuse/SampleDiffuseShader.h>


namespace Jimara {
	namespace Editor {
		CapsuleColliderGizmo::CapsuleColliderGizmo(Scene::LogicContext* context)
			: Component(context, "CapsuleColliderGizmo")
			, m_resizeHandle(Object::Instantiate<CapsuleResizeHandle>(this, Vector3(0.0f, 1.0f, 0.0f))) {
			m_resizeHandle->SetEnabled(false);
		}

		CapsuleColliderGizmo::~CapsuleColliderGizmo() {}

		void CapsuleColliderGizmo::Update() {
			Reference<CapsuleCollider> collider = Target<CapsuleCollider>();
			Reference<Transform> colliderTransform = (collider == nullptr) ? nullptr : collider->GetTransfrom();
			if (colliderTransform != nullptr && collider->ActiveInHeirarchy()) {
				const float initialRadius = collider->Radius();
				const float initialHeight = collider->Height();
				const Physics::CapsuleShape::Alignment alignment = collider->Alignment();

				auto sign = [](float f) { return (f >= 0.0f) ? 1.0f : -1.0f; };
				const Vector3 lossyScale = colliderTransform->LossyScale();
				const float radScale = Math::Max(std::abs(lossyScale.x), Math::Max(std::abs(lossyScale.y), std::abs(lossyScale.z)));
				const float midScale = sign(initialRadius) * std::abs(
					(alignment == Physics::CapsuleShape::Alignment::X) ? lossyScale.x :
					(alignment == Physics::CapsuleShape::Alignment::Y) ? lossyScale.y : lossyScale.z);

				const float initialScaledRadius = initialRadius * radScale;
				const float initailScaledHeight = initialHeight * midScale;
				float scaledRadius = initialScaledRadius;
				float scaledHeight = initailScaledHeight;

				const Vector3 rotation = Math::EulerAnglesFromMatrix(colliderTransform->WorldRotationMatrix() * (
					(alignment == Physics::CapsuleShape::Alignment::X) ? Math::MatrixFromEulerAngles(Vector3(0.0f, 0.0f, 90.0f)) :
					(alignment == Physics::CapsuleShape::Alignment::Y) ? Math::Identity() :
					(alignment == Physics::CapsuleShape::Alignment::Z) ? Math::MatrixFromEulerAngles(Vector3(90.0f, 0.0f, 0.0f)) : Math::Identity()));

				m_resizeHandle->SetEnabled(true);
				m_resizeHandle->Update(colliderTransform->WorldPosition(), rotation, scaledRadius, scaledHeight);

				auto safeDivide = [](float a, float b, float c) { return (std::abs(b) > std::numeric_limits<float>::epsilon()) ? (a / b) : c; };

				if (scaledRadius != initialScaledRadius)
					collider->SetRadius(safeDivide(scaledRadius, radScale, initialRadius));

				if (scaledHeight != initailScaledHeight)
					collider->SetHeight(safeDivide(scaledHeight, midScale, initialHeight));
			}
			else m_resizeHandle->SetEnabled(false);
		}
	}


	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::CapsuleColliderGizmo>(const Callback<const Object*>& report) {
		static const Reference<const Editor::Gizmo::ComponentConnection> connection = 
			Editor::Gizmo::ComponentConnection::Make<Editor::CapsuleColliderGizmo, CapsuleCollider>();
		report(connection);
	}
}

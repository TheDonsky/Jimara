#include "SphereColliderGizmo.h"
#include <Jimara/Components/GraphicsObjects/MeshRenderer.h>
#include <Jimara/Components/Physics/SphereCollider.h>
#include <Jimara/Data/Geometry/MeshConstants.h>
#include <Jimara/Data/Materials/SampleDiffuse/SampleDiffuseShader.h>


namespace Jimara {
	namespace Editor {
		SphereColliderGizmo::SphereColliderGizmo(Scene::LogicContext* context)
			: Component(context, "SphereColliderGizmo")
			, m_resizeHandle(Object::Instantiate<SphereResizeHandle>(this, Vector3(0.0f, 1.0f, 0.0f))) {
			m_resizeHandle->SetEnabled(false);
		}

		SphereColliderGizmo::~SphereColliderGizmo() {}

		void SphereColliderGizmo::Update() {
			Reference<SphereCollider> collider = Target<SphereCollider>();
			Reference<Transform> colliderTransform = (collider == nullptr) ? nullptr : collider->GetTransfrom();
			if (colliderTransform != nullptr && collider->ActiveInHeirarchy()) {
				const float initialRadius = collider->Radius();

				auto sign = [](float f) { return (f >= 0.0f) ? 1.0f : -1.0f; };
				const Vector3 lossyScale = colliderTransform->LossyScale();
				const float radScale = Math::Max(std::abs(lossyScale.x), Math::Max(std::abs(lossyScale.y), std::abs(lossyScale.z)));

				const float initialScaledRadius = initialRadius * radScale;
				float scaledRadius = initialScaledRadius;

				m_resizeHandle->SetEnabled(true);
				m_resizeHandle->Update(colliderTransform->WorldPosition(), colliderTransform->WorldEulerAngles(), scaledRadius);

				auto safeDivide = [](float a, float b, float c) { return (std::abs(b) > std::numeric_limits<float>::epsilon()) ? (a / b) : c; };

				if (scaledRadius != initialScaledRadius)
					collider->SetRadius(safeDivide(scaledRadius, radScale, initialRadius));
			}
			else m_resizeHandle->SetEnabled(false);
		}

		namespace {
			static const constexpr Gizmo::ComponentConnection ColliderGizmo_Connection =
				Gizmo::ComponentConnection::Make<SphereColliderGizmo, SphereCollider>();
		}
	}


	template<> void TypeIdDetails::OnRegisterType<Editor::SphereColliderGizmo>() {
		Editor::Gizmo::AddConnection(Editor::ColliderGizmo_Connection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::SphereColliderGizmo>() {
		Editor::Gizmo::RemoveConnection(Editor::ColliderGizmo_Connection);
	}
}

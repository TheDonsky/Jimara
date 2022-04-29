#include "CameraGizmo.h"
#include <Data/Generators/MeshGenerator.h>
#include <Data/Generators/MeshModifiers.h>
#include <Components/Camera.h>
#include <Components/GraphicsObjects/MeshRenderer.h>


namespace Jimara {
	namespace Editor {
		namespace {
			static const Reference<TriMesh> CAMERA_SHAPE = [&]()->Reference<TriMesh> {
				const Matrix4 forwardRotation = Math::MatrixFromEulerAngles(Vector3(90.0f, 0.0f, 0.0f));

				const Vector3 boxHalfSize(0.075f, 0.075f, 0.045f);
				const Reference<TriMesh> base = GenerateMesh::Tri::Box(-boxHalfSize, boxHalfSize);

				const float lenseRadius = 0.05f;
				const float lenseheight = 0.15f;
				const Reference<TriMesh> lense = GenerateMesh::Tri::Cylinder(
					Vector3(0.0f, boxHalfSize.z - 0.0001f + lenseheight * 0.5f, 0.0f), lenseRadius, lenseheight, 24);
				const Reference<TriMesh> rotatedLense = ModifyMesh::Transformed(forwardRotation, lense);

				return ModifyMesh::Merge(base, rotatedLense, "Camera");
			}();

			class CameraGizmo_Handle : public virtual Handle, public virtual Transform {
			public:
				inline CameraGizmo_Handle(Component* gizmo) : Component(gizmo), Transform(gizmo, "CameraGizmo_Handle") {
					Object::Instantiate<MeshRenderer>(this, "CameraGizmo_Renderer", CAMERA_SHAPE);
				}
				inline virtual ~CameraGizmo_Handle() {}

			protected:
				inline virtual void HandleActivated() override {
					Gizmo* gizmo = GetComponentInParents<Gizmo>();
					if (gizmo == nullptr || gizmo->TargetCount() <= 0) return;
					Component* target = gizmo->TargetComponent();
					if (Context()->Input()->KeyPressed(OS::Input::KeyCode::LEFT_CONTROL)
						|| Context()->Input()->KeyPressed(OS::Input::KeyCode::RIGHT_CONTROL))
						GizmoContext()->Selection()->Select(target);
					else if (Context()->Input()->KeyPressed(OS::Input::KeyCode::LEFT_ALT)
						|| Context()->Input()->KeyPressed(OS::Input::KeyCode::RIGHT_ALT))
						GizmoContext()->Selection()->Deselect(target);
					else {
						GizmoContext()->Selection()->DeselectAll();
						GizmoContext()->Selection()->Select(target);
					}
				}
			};
		}

		CameraGizmo::CameraGizmo(Scene::LogicContext* context) 
			: Component(context, "CameraGizmo")
			, m_handle(Object::Instantiate<CameraGizmo_Handle>(this)) {}

		CameraGizmo::~CameraGizmo() {}

		void CameraGizmo::Update() {
			Transform* gizmoTransform = dynamic_cast<Transform*>(m_handle.operator->());
			Component* target = TargetComponent();
			if (target == nullptr) return;
			Transform* targetTransform = target->GetTransfrom();
			if (targetTransform != nullptr && target->ActiveInHeirarchy()) {
				gizmoTransform->SetEnabled(true);
				gizmoTransform->SetWorldPosition(targetTransform->WorldPosition());
				gizmoTransform->SetWorldEulerAngles(targetTransform->WorldEulerAngles());
				gizmoTransform->SetLocalScale(Vector3(GizmoContext()->Viewport()->GizmoSizeAt(gizmoTransform->WorldPosition())));
			}
			else gizmoTransform->SetEnabled(false);
		}

		namespace {
			static const constexpr Gizmo::ComponentConnection TransformGizmo_Connection =
				Gizmo::ComponentConnection::Make<CameraGizmo, Camera>(
					Gizmo::FilterFlag::CREATE_IF_SELECTED |
					Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED |
					Gizmo::FilterFlag::CREATE_CHILD_GIZMOS_IF_SELECTED |
					Gizmo::FilterFlag::CREATE_PARENT_GIZMOS_IF_SELECTED);
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::CameraGizmo>() {
		Editor::Gizmo::AddConnection(Editor::TransformGizmo_Connection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::CameraGizmo>() {
		Editor::Gizmo::RemoveConnection(Editor::TransformGizmo_Connection);
	}
}

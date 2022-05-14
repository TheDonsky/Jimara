#pragma once
#include "../Gizmo.h"
#include "../Handles/Compound/TripleAxisMoveHandle.h"
#include "../Handles/Compound/TripleAxisRotationHandle.h"
#include "../Handles/Compound/TripleAxisScalehandle.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::TransformGizmo);

		/// <summary>
		/// Gizmo for TRansform components
		/// </summary>
		class TransformGizmo 
			: public virtual Gizmo
			, public virtual GizmoGUI::Drawer
			, Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo scene context </param>
			TransformGizmo(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~TransformGizmo();

		protected:
			/// <summary> Lets the user select between transform, scale and rotation gizmos </summary>
			virtual void OnDrawGizmoGUI()override;

			/// <summary> Updates gizmo </summary>
			virtual void Update()override;

			/// <summary> Invoked, when the gizmo gets deleted </summary>
			virtual void OnComponentDestroyed()override;

		private:
			// Underlying transform handles
			const Reference<TripleAxisMoveHandle> m_moveHandle;
			const Reference<TripleAxisRotationHandle> m_rotationHandle;
			const Reference<TripleAxisScalehandle> m_scaleHandle;

			// Action data
			struct TargetData {
				Reference<Transform> target;
				Vector3 initialPosition = Vector3(0.0f);
				Matrix4 initialRotation = Math::Identity();
				Vector3 initialLossyScale = Vector3(1.0f);
				inline TargetData() {}
				TargetData(Transform* t);
			};
			std::vector<TargetData> m_targetData;
			void FillTargetData();

			// Move handle callbacks:
			void OnMoveStarted(TripleAxisMoveHandle*);
			void OnMove(TripleAxisMoveHandle*);
			void OnMoveEnded(TripleAxisMoveHandle*);

			// Rotation handle callbacks:
			void OnRotationStarted(TripleAxisRotationHandle*);
			void OnRotation(TripleAxisRotationHandle*);
			void OnRotationEnded(TripleAxisRotationHandle*);

			// Scale handle callbacks:
			void OnScaleStarted(TripleAxisScalehandle*);
			void OnScale(TripleAxisScalehandle*);
			void OnScaleEnded(TripleAxisScalehandle*);
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::TransformGizmo>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::TransformGizmo>();
}

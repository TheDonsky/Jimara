#pragma once
#include "../Gizmo.h"
#include "../Handles/Compound/TripleAxisMoveHandle.h"
#include "../Handles/Compound/TripleAxisRotationHandle.h"
#include "../Handles/Compound/TripleAxisScalehandle.h"
#include "../Settings/TransformHandleSettings.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::TransformGizmo);

		/// <summary>
		/// Gizmo for Transform components
		/// </summary>
		class TransformGizmo 
			: public virtual Gizmo
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
			/// <summary> Updates gizmo </summary>
			virtual void Update()override;

			/// <summary> Invoked, when the gizmo gets deleted </summary>
			virtual void OnComponentDestroyed()override;

		private:
			// Settings
			const Reference<TransformHandleSettings> m_settings;

			// Underlying transform handles
			const Reference<TripleAxisMoveHandle> m_moveHandle;
			const Reference<TripleAxisRotationHandle> m_rotationHandle;
			const Reference<TripleAxisScalehandle> m_scaleHandle;

			// Action data
			Matrix4 m_initialHandleRotation = Math::Identity();
			struct TargetData {
				Reference<Transform> target;
				Vector3 initialPosition = Vector3(0.0f);
				Matrix4 initialRotation = Math::Identity();
				Vector3 initialLocalScale = Vector3(1.0f);
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

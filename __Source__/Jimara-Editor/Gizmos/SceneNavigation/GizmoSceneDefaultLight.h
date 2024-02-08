#pragma once
#include "../Gizmo.h"
#include <Jimara/Environment/Rendering/SceneObjects/Lights/LightDescriptor.h>


namespace Jimara {
	namespace Editor {
		/// <summary> Let the registry know about GizmoSceneDefaultLight </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::GizmoSceneDefaultLight);

		/// <summary>
		/// Default light that illuminates scene in SceneView when the scene does not contain any lights
		/// </summary>
		struct GizmoSceneDefaultLight : public virtual Gizmo, public virtual Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo context </param>
			GizmoSceneDefaultLight(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~GizmoSceneDefaultLight();

		protected:
			/// <summary> Update function </summary>
			virtual void Update()override;

		private:
			// Set of all lights from the target scene
			Reference<LightDescriptor::Set> m_targetSceneLights;

			// Underlying light descriptor
			Reference<LightDescriptor::Set::ItemOwner> m_lightDescriptor;

			// Private stuff resides in here
			struct Helpers;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::GizmoSceneDefaultLight>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::GizmoSceneDefaultLight>();
}

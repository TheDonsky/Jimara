#pragma once
#include "../Gizmo.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the registry know about SceneViewHotKeyOperations </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::SceneViewHotKeyOperations);

		/// <summary>
		/// Gizmo, responsible for built-in hot-key operations on scene view
		/// </summary>
		class SceneViewHotKeyOperations 
			: public virtual Gizmo
			, public virtual Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo context </param>
			SceneViewHotKeyOperations(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~SceneViewHotKeyOperations();

		protected:
			/// <summary> Update function </summary>
			virtual void Update()override;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::SceneViewHotKeyOperations>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::SceneViewHotKeyOperations>();
}


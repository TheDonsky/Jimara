#pragma once
#include "../Gizmo.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the registry know about SceneViewSelection </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::SceneViewClipboardOperations);

		/// <summary>
		/// Gizmo, responsible Clipboard operations on scene view
		/// </summary>
		class SceneViewClipboardOperations 
			: public virtual Gizmo
			, public virtual Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo context </param>
			SceneViewClipboardOperations(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~SceneViewClipboardOperations();

		protected:
			/// <summary> Update function </summary>
			virtual void Update()override;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::SceneViewClipboardOperations>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::SceneViewClipboardOperations>();
}


#pragma once
#include "../Gizmo.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the registry know about FocusOnSelectionAction </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::FocusOnSelectionAction);

		/// <summary> Component, responsible for focusing on selection boundary when corresponding key is pressed </summary>
		class FocusOnSelectionAction : public virtual Gizmo, public virtual Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo context </param>
			FocusOnSelectionAction(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~FocusOnSelectionAction();

			/// <summary> Key that causes to focus on selection on click </summary>
			inline static constexpr const OS::Input::KeyCode KEY = OS::Input::KeyCode::F;

		protected:
			/// <summary> Update function </summary>
			virtual void Update()override;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::FocusOnSelectionAction>(const Callback<const Object*>& report);
}

#pragma once
#include "EditorSceneController.h"
#include "../EditorWindow.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the registry know about ComponentInspector </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::ComponentInspector);

		/// <summary>
		/// 'Inspector' for an individual component
		/// </summary>
		class ComponentInspector : public virtual EditorSceneController, public virtual EditorWindow {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Editor context </param>
			/// <param name="targetComponent"> Target component </param>
			ComponentInspector(EditorContext* context, Component* targetComponent = nullptr);

			/// <summary> Virtual destructor </summary>
			virtual ~ComponentInspector();

			/// <summary> Target component </summary>
			Reference<Component> Target()const;

			/// <summary>
			/// Alters target component
			/// </summary>
			/// <param name="target"> Target component to use </param>
			void SetTarget(Component* target);

		protected:
			/// <summary> Draws Editor window </summary>
			virtual void DrawEditorWindow() final override;

		private:
			// Lock for target component reference
			mutable SpinLock m_componentLock;

			// Target component
			Reference<Component> m_component;

			// Invoked, when the target component goes out of scope
			void OnComponentDestroyed(Component* component);
		};
	}

	// TypeIdDetails for ComponentInspector
	template<> void TypeIdDetails::GetParentTypesOf<Editor::ComponentInspector>(const Callback<TypeId>& report);
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::ComponentInspector>(const Callback<const Object*>& report);
}

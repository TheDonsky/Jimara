#pragma once
#include "EditorSceneController.h"
#include "../EditorWindow.h"


namespace Jimara {
	namespace Editor {
		class ComponentInspector : public virtual EditorSceneController, public virtual EditorWindow {
		public:
			ComponentInspector(EditorContext* context, Component* targetComponent = nullptr);

			virtual ~ComponentInspector();

			Reference<Component> Target()const;

			void SetTarget(Component* target);

		protected:
			virtual void DrawEditorWindow() final override;

		private:
			mutable SpinLock m_componentLock;
			Reference<Component> m_component;

			void OnComponentDestroyed(Component* component);
		};
	}

	template<> void TypeIdDetails::GetParentTypesOf<Editor::ComponentInspector>(const Callback<TypeId>& report);
}

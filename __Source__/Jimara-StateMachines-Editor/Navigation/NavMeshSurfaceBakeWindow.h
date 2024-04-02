#pragma once
#include "../Types.h"
#include <Jimara-Editor/EditorWindows/EditorWindow.h>
#include <Jimara-StateMachines/Navigation/NavMesh/NavMeshBaker.h>


namespace Jimara {
	namespace Editor {
		JIMARA_REGISTER_TYPE(Jimara::Editor::NavMeshSurfaceBakeWindow);

		class JIMARA_STATE_MACHINES_EDITOR_API NavMeshSurfaceBakeWindow : public virtual EditorWindow {
		public:
			NavMeshSurfaceBakeWindow(EditorContext* context);

			virtual ~NavMeshSurfaceBakeWindow();

		protected:
			virtual void DrawEditorWindow() override;

		private:
			WeakReference<Component> m_root;
			NavMeshBaker::Settings m_settings;

			Reference<Object> m_bakeProcess;
			struct Helpers;
		};
	}

	// Registration callbacks
	template<> JIMARA_STATE_MACHINES_EDITOR_API void TypeIdDetails::GetTypeAttributesOf<Editor::NavMeshSurfaceBakeWindow>(const Callback<const Object*>& report);
}

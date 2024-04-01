#pragma once
#include "../Types.h"
#include <Jimara-Editor/EditorWindows/EditorWindow.h>


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
			
		};
	}

	// Registration callbacks
	template<> JIMARA_STATE_MACHINES_EDITOR_API void TypeIdDetails::GetTypeAttributesOf<Editor::NavMeshSurfaceBakeWindow>(const Callback<const Object*>& report);
}

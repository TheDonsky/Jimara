#pragma once
#include "../Types.h"
#include <Jimara-Editor/EditorWindows/EditorWindow.h>
#include <Jimara-StateMachines/Navigation/NavMesh/NavMeshBaker.h>


namespace Jimara {
	namespace Editor {
		/// <summary> Let's let the system know this window is a thing </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::NavMeshSurfaceBakeWindow);

		/// <summary>
		/// Editor window for Navigation Mesh baking
		/// </summary>
		class JIMARA_STATE_MACHINES_EDITOR_API NavMeshSurfaceBakeWindow : public virtual EditorWindow {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Editor context </param>
			NavMeshSurfaceBakeWindow(EditorContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~NavMeshSurfaceBakeWindow();

		protected:
			/// <summary> Draws Editor window </summary>
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

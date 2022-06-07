#pragma once
#include "EditorSceneController.h"
#include "../EditorWindow.h"


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// If EditorWindow has to draw something scene-related, overriding this is adviced to avoid excessive performance penalty.
		/// </summary>
		class EditorSceneWindow : public virtual EditorSceneController, public virtual EditorWindow {
		public:
			/// <summary> Virtual destructor </summary>
			inline virtual ~EditorSceneWindow() = 0;

		protected:
			/// <summary> Draws the contents of the editor window </summary>
			virtual void CreateEditorWindow()final override {
				typedef void(*JobFn)(Object*, EditorScene*);
				Reference<EditorScene> scene = GetOrCreateScene();
				if (scene == nullptr) return;
				static const JobFn executeUnderCommonLock = [](Object* selfPtr, EditorScene* scene) {
					if (scene == nullptr) return;
					EditorSceneWindow* window = dynamic_cast<EditorSceneWindow*>(selfPtr);
					assert(window != nullptr);
					window->EditorWindow::CreateEditorWindow();
				};
				scene->ExecuteOnImGuiThread(Callback(executeUnderCommonLock), this);
			}
		};

		/// <summary> Virtual destructor </summary>
		inline EditorSceneWindow::~EditorSceneWindow() {}
	}

	// TypeIdDetails for EditorSceneWindow
	template<> inline void TypeIdDetails::GetParentTypesOf<Editor::EditorSceneWindow>(const Callback<TypeId>& report) { 
		report(TypeId::Of<Editor::EditorSceneController>());
		report(TypeId::Of<Editor::EditorWindow>());
	}
}

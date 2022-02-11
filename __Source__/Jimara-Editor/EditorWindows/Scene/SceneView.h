#pragma once
#include "EditorSceneController.h"
#include "../EditorWindow.h"
#include <Environment/GraphicsContext/LightingModels/ObjectIdRenderer/ViewportObjectQuery.h>


namespace Jimara {
	namespace Editor {
		JIMARA_REGISTER_TYPE(Jimara::Editor::SceneView);

		class SceneView : public virtual EditorSceneController, public virtual EditorWindow {
		public:
			SceneView(EditorContext* context);

			virtual ~SceneView();

		protected:
			virtual void DrawEditorWindow() final override;

		private:
			Reference<Scene::LogicContext> m_viewContext;
			Reference<JobSystem::Job> m_updateJob;
		};
	}

	template<> void TypeIdDetails::GetParentTypesOf<Editor::SceneView>(const Callback<TypeId>& report);
	template<> void TypeIdDetails::OnRegisterType<Editor::SceneView>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::SceneView>();
}

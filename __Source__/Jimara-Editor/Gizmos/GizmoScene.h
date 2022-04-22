#pragma once
#include "../Environment/EditorScene.h"


namespace Jimara {
	namespace Editor {
		class GizmoScene : public virtual Object {
		public:
			class Context : public virtual Object {
			public:
				static Reference<Context> Of(Scene::LogicContext* gizmoScene);

			private:
			};

			static Reference<GizmoScene> Create(EditorScene* editorScene);

			virtual ~GizmoScene();

		private:
			const Reference<EditorScene> m_editorScene;
			const Reference<Scene> m_gizmoScene;
			const Reference<Context> m_context;
		};
	}
}

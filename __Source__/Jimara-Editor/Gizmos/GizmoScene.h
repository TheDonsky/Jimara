#pragma once
#include "../Environment/EditorScene.h"


namespace Jimara {
	namespace Editor {
		class GizmoScene : public virtual Object {
		public:
			class Context : public virtual Object {
			public:

			private:
				mutable SpinLock m_ownerLock;
				GizmoScene* m_owner;

				inline Context(GizmoScene* owner) : m_owner(owner) {}
				Reference<GizmoScene> GetOwner()const;

				friend class GizmoScene;
			};

			static Reference<Context> GetContext(Scene::LogicContext* gizmoScene);

			static Reference<GizmoScene> Create(EditorScene* editorScene);

			virtual ~GizmoScene();

		private:
			const Reference<EditorScene> m_editorScene;
			const Reference<Scene> m_gizmoScene;
			const Reference<Context> m_context;
			
			std::unordered_set<Reference<Component>> m_deselectedComponents;

			GizmoScene(EditorScene* editorScene, Scene* gizmoScene);
			
			void Update();
			void OnComponentCreated(Component* component);
			void OnComponentDestroyed(Component* component);
		};
	}
}

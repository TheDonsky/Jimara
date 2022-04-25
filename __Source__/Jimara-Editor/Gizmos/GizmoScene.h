#pragma once
#include "../Environment/EditorScene.h"
#include "GizmoViewport.h"


namespace Jimara {
	namespace Editor {
		class GizmoScene : public virtual Object {
		public:
			class Context : public virtual Object {
			public:
				inline Scene::LogicContext* TargetContext()const { return m_targetContext; }

				inline Scene::LogicContext* GizmoContext()const { return m_gizmoContext; }

				inline SceneSelection* Selection()const { return m_selection; }

				inline GizmoViewport* Viewport()const { return m_viewport; }

			private:
				const Reference<Scene::LogicContext> m_targetContext;
				const Reference<Scene::LogicContext> m_gizmoContext;
				const Reference<SceneSelection> m_selection;
				const Reference<GizmoViewport> m_viewport;

				mutable SpinLock m_ownerLock;
				GizmoScene* m_owner = nullptr;

				inline Context(
					Scene::LogicContext* targetContext, Scene::LogicContext* gizmoContext, 
					SceneSelection* selection, GizmoScene* owner)
					: m_targetContext(targetContext), m_gizmoContext(gizmoContext)
					, m_selection(selection), m_owner(owner)
					, m_viewport(Object::Instantiate<GizmoViewport>(targetContext, gizmoContext)) {}
				Reference<GizmoScene> GetOwner()const;

				friend class GizmoScene;
			};

			static Reference<Context> GetContext(Scene::LogicContext* gizmoScene);

			static Reference<GizmoScene> Create(EditorScene* editorScene);

			virtual ~GizmoScene();

			inline Context* GetContext()const { return m_context; }

			inline EditorInput* Input()const { return m_editorInput; }

		private:
			const Reference<EditorScene> m_editorScene;
			const Reference<Scene> m_gizmoScene;
			const Reference<Context> m_context;
			const Reference<EditorInput> m_editorInput;
			
			std::unordered_set<Reference<Component>> m_deselectedComponents;

			GizmoScene(EditorScene* editorScene, Scene* gizmoScene, EditorInput* input);
			
			void Update();
			void OnComponentCreated(Component* component);
			void OnComponentDestroyed(Component* component);
		};
	}
}

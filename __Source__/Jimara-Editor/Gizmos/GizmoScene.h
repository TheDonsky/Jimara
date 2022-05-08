#pragma once
#include "../Environment/EditorScene.h"
#include "GizmoViewport.h"
#include "GizmoGUI.h"


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Scene for Gizmos, created per-SceneView
		/// </summary>
		class GizmoScene : public virtual Object {
		public:
			/// <summary>
			/// Gizmo scene context
			/// </summary>
			class Context : public virtual Object {
			public:
				/// <summary> Scene context, the gizmos are targetting </summary>
				inline Scene::LogicContext* TargetContext()const { return m_targetContext; }

				/// <summary> Separate context of gizmos (Gizmo components are spowned here) </summary>
				inline Scene::LogicContext* GizmoContext()const { return m_gizmoContext; }

				/// <summary> Target scene selection manager </summary>
				inline SceneSelection* Selection()const { return m_selection; }

				/// <summary> Main viewport of the GizmoScene </summary>
				inline GizmoViewport* Viewport()const { return m_viewport; }

				/// <summary>
				/// Keeps track of Component changes (for Undo actions)
				/// </summary>
				/// <param name="component"> Component to track </param>
				/// <param name="trackChildren"> If true, child components will be tracked recursively </param>
				inline void TrackComponent(Component* component, bool trackChildren)const {
					Reference<GizmoScene> owner = GetOwner();
					if (owner != nullptr) owner->m_editorScene->TrackComponent(component, trackChildren);
				}

			private:
				// Target context
				const Reference<Scene::LogicContext> m_targetContext;

				// Gizmo scene context
				const Reference<Scene::LogicContext> m_gizmoContext;

				// Target scene selection
				const Reference<SceneSelection> m_selection;

				// Main gizmo viewport
				const Reference<GizmoViewport> m_viewport;

				// Owner
				mutable SpinLock m_ownerLock;
				GizmoScene* m_owner = nullptr;

				// Constructor
				inline Context(
					Scene::LogicContext* targetContext, Scene::LogicContext* gizmoContext, 
					SceneSelection* selection, GizmoScene* owner)
					: m_targetContext(targetContext), m_gizmoContext(gizmoContext)
					, m_selection(selection), m_owner(owner)
					, m_viewport(Object::Instantiate<GizmoViewport>(targetContext, gizmoContext)) {}
				Reference<GizmoScene> GetOwner()const;

				// Only the owner can create GizmoScene Context
				friend class GizmoScene;
			};

			/// <summary>
			/// Gets GizmoScene::Context from it's corresponding scene context
			/// </summary>
			/// <param name="gizmoContext"> Gizmos' logic context </param>
			/// <returns> Context c, such that c->GizmoContext() is gizmoContext, if given context is that of a GizmoScene </returns>
			static Reference<Context> GetContext(Scene::LogicContext* gizmoContext);

			/// <summary>
			/// Creates a gizmo context for an editor scene
			/// </summary>
			/// <param name="editorScene"> Editor scene to target </param>
			/// <returns> Instance of a new GizmoScene </returns>
			static Reference<GizmoScene> Create(EditorScene* editorScene);

			/// <summary> Virtual destructor </summary>
			virtual ~GizmoScene();

			/// <summary> Corresponding context </summary>
			inline Context* GetContext()const { return m_context; }

			/// <summary> Input module of the GizmoScene </summary>
			inline EditorInput* Input()const { return m_editorInput; }

			/// <summary> Draws content form GizmoGUI::Drawer objects </summary>
			void DrawGizmoGUI()const;

		private:
			// Target Editor Scene
			const Reference<EditorScene> m_editorScene;

			// Gizmo scene
			const Reference<Scene> m_gizmoScene;

			// GizmoScene Context
			const Reference<Context> m_context;

			// Input module, used by gizmo scene
			const Reference<EditorInput> m_editorInput;

			// Gizmo GUI module
			const Reference<GizmoGUI> m_gizmoGUI;

			// Gizmo creator
			Reference<Object> m_gizmoCreator;

			// Constructor
			GizmoScene(EditorScene* editorScene, Scene* gizmoScene, EditorInput* input, GizmoGUI* gizmoGUI);

			// Updates gizmo scene
			void Update();
		};
	}
}

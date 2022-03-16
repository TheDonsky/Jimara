#pragma once
#include "UndoManager.h"
#include <Environment/Scene/Scene.h>

namespace Jimara {
	namespace Editor {
		class SceneUndoManager : public virtual Object {
		public:
			SceneUndoManager(Scene* scene, UndoManager* undoManager);

			virtual ~SceneUndoManager();

			Scene::LogicContext* SceneContext()const;

			void TrackComponent(Component* component, bool trackChildren);

			void Flush();

		private:
			const Reference<Scene> m_scene;
			const Reference<UndoManager> m_undoManager;
			std::unordered_set<Reference<Component>> m_trackedComponents;
			std::unordered_map<Reference<Component>, GUID> m_componentIds;
			std::unordered_map<GUID, Reference<Object>> m_componentStates;

			void OnComponentDestroyed(Component* component);
		};
	}
}

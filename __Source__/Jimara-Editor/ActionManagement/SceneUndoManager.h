#pragma once
#include "UndoManager.h"
#include <Environment/Scene/Scene.h>
#include <Data/Serialization/Helpers/SerializeToJson.h>

namespace Jimara {
	namespace Editor {
		class SceneUndoManager : public virtual Object {
		public:
			SceneUndoManager(Scene::LogicContext* context);

			virtual ~SceneUndoManager();

			Scene::LogicContext* SceneContext()const;

			void TrackComponent(Component* component, bool trackChildren);

			Reference<UndoManager::Action> Flush();

			void Discard();

		private:
			const Reference<Scene::LogicContext> m_context;
			const GUID m_rootGUID = GUID::Generate();

			struct ComponentData : public virtual Object {
				std::string componentType;
				GUID guid = {};
				GUID parentId = {};
				size_t indexInParent = 0;
				std::unordered_set<GUID> referencingObjects;
				std::unordered_set<GUID> referencedObjects;
				nlohmann::json serializedData;
			};
			struct ComponentDataChange {
				Reference<ComponentData> oldData;
				Reference<ComponentData> newData;
			};

			std::unordered_set<Reference<Component>> m_trackedComponents;

			std::unordered_map<Reference<Component>, GUID> m_componentIds;
			std::unordered_map<GUID, Reference<Component>> m_idsToComponents;
			std::unordered_map<GUID, Reference<ComponentData>> m_componentStates;

			EventInstance<> m_onDiscard;

			class UndoAction;

			void OnComponentDestroyed(Component* component);

			bool RefreshRootReference();
			GUID GetGuid(Component* component);
			ComponentDataChange UpdateComponentData(Component* component, const ComponentSerializer::Set* serializers);
			void UpdateReferencingObjects(const ComponentData* oldData, const ComponentData* newData);
		};
	}
}

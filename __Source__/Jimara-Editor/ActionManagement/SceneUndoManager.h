#pragma once
#include "UndoManager.h"
#include <Environment/Scene/Scene.h>
#include <Data/Serialization/Helpers/SerializeToJson.h>

namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Object responsible for tracking scene graph changes
		/// </summary>
		class SceneUndoManager : public virtual Object {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Target scene context </param>
			SceneUndoManager(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~SceneUndoManager();

			/// <summary> Target scene context </summary>
			Scene::LogicContext* SceneContext()const;

			/// <summary>
			/// SceneUndoManager does not automagically know which Components are dirty;
			/// User needs to tell it explicitly, that an object may be changed by the end of frame using this call.
			/// <para /> Note: Requered when changing Component internals, when it's created/destroyed or it's parent/childId gets changed
			/// </summary>
			/// <param name="component"> Component to track </param>
			/// <param name="trackChildren"> If true, entire child subtree under the component will also be tracked </param>
			void TrackComponent(Component* component, bool trackChildren);

			/// <summary>
			/// "Flushes" TrackComponent() calls and generates an UndoManager::Action that can referse the state
			/// </summary>
			/// <returns> UndoManager::Action if there were any real changes, nullptr otherwise </returns>
			Reference<UndoManager::Action> Flush();

			/// <summary> 
			/// Discards all existing Actions, previously generated via Flush() calls and clears all stored data
			/// </summary>
			void Discard();

		private:
			// Target scene context
			const Reference<Scene::LogicContext> m_context;

			// Root object id has a persistant GUID tied to it
			const GUID m_rootGUID = GUID::Generate();

			// Data, stored about each component
			struct ComponentData : public virtual Object {
				std::string componentType;
				GUID guid = {};
				GUID parentId = {};
				size_t indexInParent = 0;
				std::unordered_set<GUID> referencingObjects;
				std::unordered_set<GUID> referencedObjects;
				nlohmann::json serializedData;
			};

			// Change, recorded as old and new data
			struct ComponentDataChange {
				Reference<ComponentData> oldData;
				Reference<ComponentData> newData;
			};

			// Set of components that are currently tracked
			std::unordered_set<Reference<Component>> m_trackedComponents;

			// Internal data about known components
			std::unordered_map<Reference<Component>, GUID> m_componentIds;
			std::unordered_map<GUID, Reference<Component>> m_idsToComponents;
			std::unordered_map<GUID, Reference<ComponentData>> m_componentStates;

			// Invalidates Actions once invoked
			EventInstance<> m_onDiscard;

			// Internal UndoManager::Action
			class UndoAction;

			// Invoked, when a tracked component gets destroyed
			void OnComponentDestroyed(Component* component);

			// Makes sure, root object record does not get mangled up
			bool RefreshRootReference();

			// Gets or created GUID for given component
			GUID GetGuid(Component* component);

			// Updates data for given component and generates change
			ComponentDataChange UpdateComponentData(Component* component, const ComponentSerializer::Set* serializers);

			// Refreshes referencingObjects from referencedObjects
			void UpdateReferencingObjects(const ComponentData* oldData, const ComponentData* newData);
		};
	}
}

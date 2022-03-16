#include "SceneUndoManager.h"
#include <Data/Serialization/Helpers/SerializeToJson.h>

namespace Jimara {
	namespace Editor {
		namespace {
			struct ComponentData : public virtual Object {
				Reference<const ComponentSerializer> serializer;
				GUID guid;
				GUID parentId;
				size_t indexInParent;
				std::unordered_set<GUID> referencedObjects;
				std::unordered_set<GUID> referencingObjects;
				nlohmann::json serializedData;
			};
		}

		SceneUndoManager::SceneUndoManager(Scene* scene, UndoManager* undoManager)
			: m_scene(scene)
			, m_undoManager([&]() -> Reference<UndoManager> {
			if (undoManager == nullptr)
				return Object::Instantiate<UndoManager>();
			else return undoManager;
				}()) {
			assert(scene != nullptr);
			assert(undoManager != nullptr);
		}

		SceneUndoManager::~SceneUndoManager() {}

		Scene::LogicContext* SceneUndoManager::SceneContext()const { return m_scene->Context(); }

		void SceneUndoManager::TrackComponent(Component* component, bool trackChildren) {
			std::unique_lock<std::recursive_mutex> lock(SceneContext()->UpdateLock());
			// Check if the component is a part of the main tree:
			{
				if (component == nullptr) return;
				Component* const rootObject = SceneContext()->RootObject();
				Component* ptr = component;
				while (true) {
					Component* parent = ptr->Parent();
					if (parent == nullptr) break;
					else ptr = parent;
				}
				if (ptr != rootObject) return;
			}

			// Determine the list of all tracked objects:
			{
				static thread_local std::vector<Component*> components;
				components.push_back(component);
				if (trackChildren)
					component->GetComponentsInChildren<Component>(components, true);
				for (size_t i = 0; i < components.size(); i++)
					m_trackedComponents.insert(components[i]);
				components.clear();
			}
		}

		void SceneUndoManager::Flush() {
			std::unique_lock<std::recursive_mutex> lock(SceneContext()->UpdateLock());
			const Reference<ComponentSerializer::Set> serializers = ComponentSerializer::Set::All();

			// Get id for component:
			auto getGuid = [&](Component* component) -> GUID {
				if (component == nullptr || component == SceneContext()->RootObject()) return {};
				decltype(m_componentIds)::const_iterator it = m_componentIds.find(component);
				if (it != m_componentIds.end()) return it->second;
				GUID id = GUID::Generate();
				m_componentIds[component] = id;
				if (!component->Destroyed())
					component->OnDestroyed() += Callback(&SceneUndoManager::OnComponentDestroyed, this);
				return id;
			};

			// Get ComponentData:
			auto getComponentData = [&](Component* component) -> Reference<ComponentData> {
				GUID guid = getGuid(component);
				decltype(m_componentStates)::const_iterator it = m_componentStates.find(guid);
				if (it != m_componentStates.end()) {
					Reference<ComponentData> data = it->second;
					if (data == nullptr)
						m_scene->Context()->Log()->Fatal("SceneUndoManager::Flush - Internal error: Stored data type incorrect! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					else if (data->guid != guid)
						m_scene->Context()->Log()->Fatal("SceneUndoManager::Flush - Internal error: GUID mismatch! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return data;
				}
				else if (component->Destroyed()) return nullptr;
				else {
					Reference<ComponentData> data = Object::Instantiate<ComponentData>();
					data->serializer = [&]() {
						Reference<const ComponentSerializer> serializer = serializers->FindSerializerOf(component);
						if (serializer == nullptr) serializer = TypeId::Of<Component>().FindAttributeOfType<ComponentSerializer>();
						assert(serializer != nullptr);
						return serializer;
					}();
					data->guid = guid;
					data->parentId = getGuid(component->Parent());
					data->indexInParent = component->IndexInParent();
					data->referencedObjects = {}; // __TODO__: Serialize and find referenced objects....
					data->referencingObjects = {}; // Probably should stay as is... We'll fill this in automagically when some objects with a reference to this one get found
					data->serializedData = {}; // __TODO__: Fill this crap in with a json-serialization...
					m_componentStates[guid] = data;
					return data;
				}
			};

			for (decltype(m_trackedComponents)::const_iterator it = m_trackedComponents.begin(); it != m_trackedComponents.end(); ++it) {
				Reference<ComponentData> data = getComponentData(*it);

				// No data was/is stored (ei maybe something was created and deleted right away)...
				if (data == nullptr) {
					(*it)->OnDestroyed() -= Callback(&SceneUndoManager::OnComponentDestroyed, this);
					m_componentIds.erase(*it);
					continue;
				}
			}

			// __TODO__: Implement this crap!
			m_scene->Context()->Log()->Error("SceneUndoManager::Flush - Not [Yet] implemented!");
			m_trackedComponents.clear();
		}

		void SceneUndoManager::OnComponentDestroyed(Component* component) {
			std::unique_lock<std::recursive_mutex> lock(SceneContext()->UpdateLock());
			component->OnDestroyed() -= Callback(&SceneUndoManager::OnComponentDestroyed, this);
			m_trackedComponents.insert(component);
		}
	}
}

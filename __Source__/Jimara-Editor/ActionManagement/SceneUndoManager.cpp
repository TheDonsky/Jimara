#include "SceneUndoManager.h"

namespace Jimara {
	namespace Editor {
		namespace {
			// Check if the component is a part of the main tree:
			inline static bool CanTrackComponent(Component* component, const Scene::LogicContext* context) {
				if (component == nullptr) return false;
				Component* const rootObject = context->RootObject();
				Component* ptr = component;
				while (true) {
					Component* parent = ptr->Parent();
					if (parent == nullptr) break;
					else ptr = parent;
				}
				return (ptr == rootObject);
			}
		}

		class SceneUndoManager::UndoAction : public virtual UndoManager::Action {
		private:
			const Reference<SceneUndoManager> m_owner;
			const std::vector<ComponentDataChange> m_changes;

		public:
			UndoAction(SceneUndoManager* owner, std::vector<ComponentDataChange>&& changes) 
				: m_owner(owner), m_changes(std::move(changes)) {}

			virtual ~UndoAction() {}

		protected:
			virtual void Undo() final override {
				std::unique_lock<std::recursive_mutex> lock(m_owner->SceneContext()->UpdateLock());
				
				for (size_t i = 0; i < m_changes.size(); i++) {
					const ComponentDataChange& change = m_changes[i];
					if (change.oldData == nullptr) {
						// __TODO__: Destroy component instance if it exists!
						continue;
					}
					else if (change.newData == nullptr) {
						// __TODO__: Create component instance (make roor object a parent for now, since previous parent might be deleted as well)!
					}
				}
				
				for (size_t i = 0; i < m_changes.size(); i++) {
					const ComponentDataChange& change = m_changes[i];
					if (change.newData == nullptr && change.oldData != nullptr) {
						// __TODO__: Update component parent and deal with the child order!
					}
				}

				for (size_t i = 0; i < m_changes.size(); i++) {
					const ComponentDataChange& change = m_changes[i];
					ComponentData* data = (change.newData != nullptr) ? change.newData : change.oldData;
					// __TODO__: Apply data!
				}

				m_owner->SceneContext()->Log()->Error("SceneUndoManager::UndoAction::Undo - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}
		};

		SceneUndoManager::SceneUndoManager(Scene* scene) : m_scene(scene) {
			assert(scene != nullptr);
		}

		SceneUndoManager::~SceneUndoManager() {}

		Scene::LogicContext* SceneUndoManager::SceneContext()const { return m_scene->Context(); }

		void SceneUndoManager::TrackComponent(Component* component, bool trackChildren) {
			std::unique_lock<std::recursive_mutex> lock(SceneContext()->UpdateLock());
			if (!CanTrackComponent(component, SceneContext())) return;

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

		Reference<UndoManager::Action> SceneUndoManager::Flush() {
			std::unique_lock<std::recursive_mutex> lock(SceneContext()->UpdateLock());
			const Reference<ComponentSerializer::Set> serializers = ComponentSerializer::Set::All();

			// Update internal state:
			std::vector<ComponentDataChange> changes;
			for (decltype(m_trackedComponents)::const_iterator it = m_trackedComponents.begin(); it != m_trackedComponents.end(); ++it) {
				ComponentDataChange change = UpdateComponentData(*it, serializers);
				if (change.newData == nullptr && change.oldData == nullptr) continue;
				else if (change.newData != nullptr && change.oldData != nullptr) {
					if (change.newData->componentType != change.oldData->componentType)
						SceneContext()->Log()->Error("SceneUndoManager::Flush - Tracked component type mismatch!");
					// If component was tracked, but not changed, we might as well ignore it here...
					if ((change.newData->parentId == change.oldData->parentId) &&
						(change.newData->indexInParent == change.oldData->indexInParent) &&
						(change.newData->serializedData == change.oldData->serializedData)) continue;
				}
				changes.push_back(change);
			}
			m_trackedComponents.clear();

			// Update referencingObjects:
			for (size_t i = 0; i < changes.size(); i++) {
				const ComponentDataChange& change = changes[i];
				if (change.newData != nullptr)
					for (decltype(change.newData->referencedObjects)::const_iterator it = change.newData->referencedObjects.begin(); it != change.newData->referencedObjects.end(); ++it) {
						decltype(m_componentStates)::const_iterator stateIterator = m_componentStates.find(*it);
						if (stateIterator == m_componentStates.end()) continue;
						stateIterator->second->referencingObjects.insert(change.newData->guid);
					}
				if (change.oldData != nullptr)
					for (decltype(change.oldData->referencedObjects)::const_iterator it = change.oldData->referencedObjects.begin(); it != change.oldData->referencedObjects.end(); ++it) {
						decltype(m_componentStates)::const_iterator stateIterator = m_componentStates.find(*it);
						if (stateIterator == m_componentStates.end()) continue;
						if (change.newData == nullptr || change.newData->referencedObjects.find(*it) == change.newData->referencedObjects.end())
							stateIterator->second->referencingObjects.erase(change.oldData->guid);
					}
			}

			// Remove deleted GUID-s:
			for (size_t i = 0; i < changes.size(); i++) {
				const ComponentDataChange& change = changes[i];
				if (change.newData != nullptr) continue;
				decltype(m_idsToComponents)::iterator it = m_idsToComponents.find(change.oldData->guid);
				if (it == m_idsToComponents.end()) {
					SceneContext()->Log()->Warning("SceneUndoManager::Flush - Can not remove the record of the deleted component! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					continue;
				}
				if (!it->second->Destroyed()) {
					SceneContext()->Log()->Error("SceneUndoManager::Flush - Component not destroyed, but new state missing! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					continue;
				}
				m_componentIds.erase(it->second);
				m_idsToComponents.erase(it);
			}

			// Generate undo action:
			if (changes.empty()) return nullptr;
			Reference<UndoAction> action = new UndoAction(this, std::move(changes));
			action->ReleaseRef();
			return action;
		}

		void SceneUndoManager::OnComponentDestroyed(Component* component) {
			std::unique_lock<std::recursive_mutex> lock(SceneContext()->UpdateLock());
			component->OnDestroyed() -= Callback(&SceneUndoManager::OnComponentDestroyed, this);
			m_trackedComponents.insert(component);
			GUID guid = GetGuid(component);
			if (guid == (GUID{})) return;
			decltype(m_componentStates)::const_iterator stateIterator = m_componentStates.find(guid);
			if (stateIterator == m_componentStates.end()) return;
			ComponentData* data = stateIterator->second;
			for (decltype(data->referencingObjects)::const_iterator it = data->referencingObjects.begin(); it != data->referencingObjects.end(); ++it) {
				decltype(m_idsToComponents)::const_iterator componentIt = m_idsToComponents.find(*it);
				if (componentIt != m_idsToComponents.end()) m_trackedComponents.insert(componentIt->second);
				else SceneContext()->Log()->Warning("SceneUndoManager::OnComponentDestroyed - Failed to find referencing component! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}
		}


		GUID SceneUndoManager::GetGuid(Component* component) {
			if (component == nullptr) return {};
			else if (component == SceneContext()->RootObject()) return m_rootGUID;
			decltype(m_componentIds)::const_iterator it = m_componentIds.find(component);
			if (it != m_componentIds.end()) return it->second;
			else if (!component->Destroyed()) {
				GUID id = GUID::Generate();
				m_componentIds[component] = id;
				m_idsToComponents[id] = component;
				component->OnDestroyed() += Callback(&SceneUndoManager::OnComponentDestroyed, this);
				return id;
			}
			else return {};
		}

		SceneUndoManager::ComponentDataChange SceneUndoManager::UpdateComponentData(Component* component, const ComponentSerializer::Set* serializers) {
			assert(component != nullptr);
			Reference<const ComponentSerializer> serializer = serializers->FindSerializerOf(component);
			if (serializer == nullptr) serializer = TypeId::Of<Component>().FindAttributeOfType<ComponentSerializer>();
			assert(serializer != nullptr);

			const GUID guid = GetGuid(component);
			if (guid == (GUID{})) return ComponentDataChange();

			ComponentDataChange change;
			{
				decltype(m_componentStates)::iterator stateIterator = m_componentStates.find(guid);
				change.oldData = (stateIterator == m_componentStates.end()) ? nullptr : stateIterator->second;
				if (!component->Destroyed())
					change.newData = Object::Instantiate<ComponentData>();
				if (change.oldData == nullptr) m_componentStates[guid] = change.newData;
				else if (change.newData == nullptr) {
					m_componentStates.erase(stateIterator);
					return change;
				}
				else stateIterator->second = change.newData;
			}

			{
				change.newData->componentType = serializer->TargetComponentType().Name();
				change.newData->guid = guid;
			}
			{
				change.newData->parentId = GetGuid(component->Parent());
				change.newData->indexInParent = component->IndexInParent();
			}
			if (change.oldData != nullptr)
				change.newData->referencingObjects = change.oldData->referencingObjects;
			
			{
				bool error = false;
				change.newData->serializedData = Serialization::SerializeToJson(serializer->Serialize(component), SceneContext()->Log(), error,
					[&](const Serialization::SerializedObject& addr, bool& err) -> nlohmann::json {
						const Serialization::ObjectReferenceSerializer* const addrSerializer = addr.As<Serialization::ObjectReferenceSerializer>();
						if (addrSerializer == nullptr) {
							SceneContext()->Log()->Error("SceneUndoManager::CreateComponentData - unsupported serializer type! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							err = true;
							return {};
						}

						auto errorOnPointerSerializer = [&](const Serialization::SerializedObject&, bool& e) -> nlohmann::json {
							SceneContext()->Log()->Error("SceneUndoManager::CreateComponentData - GUID serializer should not have any object pointers! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							e = true;
							return {};
						};

						const Reference<Object> currentObject = addrSerializer->GetObjectValue(addr.TargetAddr());
						{
							Component* referencedComponent = dynamic_cast<Component*>(currentObject.operator->());
							if (CanTrackComponent(referencedComponent, SceneContext())) {
								static const Reference<const GUID::Serializer> guidSerializer = Object::Instantiate<GUID::Serializer>("ReferencedComponent");
								GUID id;
								if (referencedComponent->Destroyed()) id = {};
								else {
									id = GetGuid(referencedComponent);
									assert(id != (GUID{}));
								}
								if (id != guid) change.newData->referencedObjects.insert(id);
								return Serialization::SerializeToJson(guidSerializer->Serialize(id), SceneContext()->Log(), err, errorOnPointerSerializer);
							}
						}
						{
							Resource* resource = dynamic_cast<Resource*>(currentObject.operator->());
							if (resource != nullptr && resource->HasAsset()) {
								static const Reference<const GUID::Serializer> guidSerializer = Object::Instantiate<GUID::Serializer>("ReferencedResource");
								GUID id = resource->GetAsset()->Guid();
								return Serialization::SerializeToJson(guidSerializer->Serialize(id), SceneContext()->Log(), err, errorOnPointerSerializer);
							}
						}
						{
							Asset* asset = dynamic_cast<Asset*>(currentObject.operator->());
							if (asset != nullptr) {
								static const Reference<const GUID::Serializer> guidSerializer = Object::Instantiate<GUID::Serializer>("ReferencedAsset");
								GUID id = asset->Guid();
								return Serialization::SerializeToJson(guidSerializer->Serialize(id), SceneContext()->Log(), err, errorOnPointerSerializer);
							}
						}
					});
				if (error)
					SceneContext()->Log()->Error("SceneUndoManager::CreateComponentData - Component Snapshot created with errors! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			return change;
		}
	}
}

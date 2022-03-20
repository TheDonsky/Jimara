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

			static const Reference<const GUID::Serializer> GUID_SERIALIZER = Object::Instantiate<GUID::Serializer>("ReferencedObject");
		}

		class SceneUndoManager::UndoAction : public virtual UndoManager::Action {
		private:
			const Reference<SceneUndoManager> m_owner;
			const std::vector<ComponentDataChange> m_changes;

		public:
			inline UndoAction(SceneUndoManager* owner, std::vector<ComponentDataChange>&& changes)
				: m_owner(owner), m_changes(std::move(changes)) {}

			inline virtual ~UndoAction() {}

		private:
			inline Reference<Component> FindComponent(const GUID& guid) {
				decltype(m_owner->m_idsToComponents)::const_iterator it = m_owner->m_idsToComponents.find(guid);
				if (it == m_owner->m_idsToComponents.end()) {
					m_owner->SceneContext()->Log()->Error(
						"SceneUndoManager::UndoAction::FindComponent - Failed to find component! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
				}
				else return it->second;
			};

			inline void RemoveCreatedComponents() {
				for (size_t i = 0; i < m_changes.size(); i++) {
					const ComponentDataChange& change = m_changes[i];
					if (change.oldData != nullptr) continue;
					else if (change.newData == nullptr)
						m_owner->SceneContext()->Log()->Fatal(
							"SceneUndoManager::UndoAction::RemoveCreatedComponents - Internal error: both old and new data missing!  [File: ", __FILE__, "; Line: ", __LINE__, "]");

					// We do not touch the root object for now...
					if (change.newData->guid == m_owner->m_rootGUID) continue;

					// Old Data is nullptr, therefore, the component is newly created and should be erased:
					decltype(m_owner->m_idsToComponents)::iterator it = m_owner->m_idsToComponents.find(change.newData->guid);
					if (it == m_owner->m_idsToComponents.end()) {
						m_owner->SceneContext()->Log()->Warning("SceneUndoManager::UndoAction::RemoveCreatedComponents - Component should be deleted, but can not find it's reference!");
						continue;
					}
					Reference<Component> component = it->second;
					{
						// Remove all records from SceneUndoManager:
						m_owner->m_componentIds.erase(component);
						m_owner->m_idsToComponents.erase(it);
						m_owner->m_componentStates.erase(change.newData->guid);
						component->OnDestroyed() -= Callback(&SceneUndoManager::OnComponentDestroyed, m_owner.operator->());
					}
					{
						// Unlink children (they might be needed):
						if (component != m_owner->SceneContext()->RootObject())
							while (component->ChildCount() > 0)
								component->GetChild(component->ChildCount() - 1)->SetParent(m_owner->SceneContext()->RootObject());
					}
					component->Destroy();
				}
			}

			inline void CreateDeletedComponents(const ComponentSerializer::Set* serializers) {
				for (size_t i = 0; i < m_changes.size(); i++) {
					const ComponentDataChange& change = m_changes[i];
					if (change.newData != nullptr) continue;
					else if (change.oldData == nullptr)
						m_owner->SceneContext()->Log()->Fatal(
							"SceneUndoManager::UndoAction::CreateDeletedComponents - Internal error: both old and new data missing!  [File: ", __FILE__, "; Line: ", __LINE__, "]");
					
					// We do not touch the root object for now...
					if (change.oldData->guid == m_owner->m_rootGUID) continue;
					
					{
						// Make sure there are no records for GUID:
						if (m_owner->m_idsToComponents.find(change.oldData->guid) != m_owner->m_idsToComponents.end())
							m_owner->SceneContext()->Log()->Error(
								"SceneUndoManager::UndoAction::CreateDeletedComponents - Internal error: Component does not seem to be deleted!  [File: ", __FILE__, "; Line: ", __LINE__, "]");
					}

					// Find serializer:
					Reference<const ComponentSerializer> serializer = serializers->FindSerializerOf(change.oldData->componentType);
					if (serializer == nullptr) {
						m_owner->SceneContext()->Log()->Warning(
							"SceneUndoManager::UndoAction::CreateDeletedComponents - Failed to find serializer of type: '", change.oldData->componentType, 
							"'! (defaulting to a 'Component')  [File: ", __FILE__, "; Line: ", __LINE__, "]");
						serializer = TypeId::Of<Component>().FindAttributeOfType<ComponentSerializer>();
						assert(serializer != nullptr);
					}

					// Create component:
					Reference<Component> component = serializer->CreateComponent(m_owner->SceneContext()->RootObject());
					if (component == nullptr) {
						m_owner->SceneContext()->Log()->Error(
							"SceneUndoManager::UndoAction::CreateDeletedComponents - Failed to find recreate component of type: '", change.oldData->componentType,
							"'! (defaulting to a 'Component')  [File: ", __FILE__, "; Line: ", __LINE__, "]");
						continue;
					}

					// Restore component 'bindings':
					m_owner->m_componentIds[component] = change.oldData->guid;
					m_owner->m_idsToComponents[change.oldData->guid] = component;
					component->OnDestroyed() += Callback(&SceneUndoManager::OnComponentDestroyed, m_owner.operator->());
				}
			}

			inline void RestoreParentChildRelations() {
				std::unordered_set<Reference<Component>> parents;

				for (size_t i = 0; i < m_changes.size(); i++) {
					const ComponentDataChange& change = m_changes[i];
					if (change.oldData == nullptr) continue;
					
					// We do not touch the root object for now...
					if (change.oldData->guid == m_owner->m_rootGUID) continue;

					// Find component and desired parent:
					Reference<Component> component = FindComponent(change.oldData->guid);
					Reference<Component> parentComponent = FindComponent(change.oldData->parentId);

					// Update parent:
					if (component == nullptr || parentComponent == nullptr || component == parentComponent || component == m_owner->m_context->RootObject()) continue;
					component->SetParent(parentComponent);
					parents.insert(parentComponent);
				}

				for (decltype(parents)::const_iterator it = parents.begin(); it != parents.end(); ++it) {
					// __TODO__: Correct child order here...
				}

				m_owner->SceneContext()->Log()->Warning(
					"SceneUndoManager::UndoAction::RestoreParentChildRelations - Child order correction not yet implemented! ",
					"[File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			inline void RestoreSerializedData(const ComponentSerializer::Set* serializers) {
				for (size_t i = 0; i < m_changes.size(); i++) {
					const ComponentDataChange& change = m_changes[i];
					if (change.oldData == nullptr) continue;
					
					// Find component:
					const Reference<Component> component = FindComponent(change.oldData->guid);
					if (component == nullptr) {
						m_owner->SceneContext()->Log()->Error(
							"SceneUndoManager::UndoAction::RestoreSerializedData - Failed to find component! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						continue;
					}

					// Find serializer:
					Reference<const ComponentSerializer> serializer = serializers->FindSerializerOf(component);
					if (serializer == nullptr) {
						m_owner->SceneContext()->Log()->Warning(
							"SceneUndoManager::UndoAction::RestoreSerializedData - Failed to find serializer of type: '", change.oldData->componentType,
							"'! (defaulting to a 'Component')  [File: ", __FILE__, "; Line: ", __LINE__, "]");
						serializer = TypeId::Of<Component>().FindAttributeOfType<ComponentSerializer>();
						assert(serializer != nullptr);
					}

					if (!Serialization::DeserializeFromJson(serializer->Serialize(component), change.oldData->serializedData, m_owner->SceneContext()->Log(), 
						[&](const Serialization::SerializedObject& addr, const nlohmann::json& guidData) -> bool {
							const Serialization::ObjectReferenceSerializer* const addrSerializer = addr.As<Serialization::ObjectReferenceSerializer>();
							if (addrSerializer == nullptr) {
								m_owner->SceneContext()->Log()->Error(
									"SceneUndoManager::UndoAction::RestoreSerializedData - Unexpected serializer type! [File: ", __FILE__, "; Line: ", __LINE__, "]");
								return false;
							}

							// Get GUID:
							GUID objectId = {};
							if (!Serialization::DeserializeFromJson(GUID_SERIALIZER->Serialize(objectId), guidData, m_owner->SceneContext()->Log(),
								[&](const Serialization::SerializedObject&, const nlohmann::json&) -> bool {
									m_owner->SceneContext()->Log()->Error(
										"SceneUndoManager::UndoAction::RestoreSerializedData - GUID serializer should not have any object pointers! [File: ", __FILE__, "; Line: ", __LINE__, "]");
									return false;
								})) return false;
							const TypeId valueType = addrSerializer->ReferencedValueType();

							auto setValue = [&](Object* value) -> bool {
								addrSerializer->SetObjectValue(value, addr.TargetAddr());
								return true;
							};
							{
								// Check if it's a Component:
								decltype(m_owner->m_idsToComponents)::const_iterator it = m_owner->m_idsToComponents.find(objectId);
								if (it != m_owner->m_idsToComponents.end())
									if (valueType.CheckType(it->second))
										return setValue(it->second);
							}
							{
								// Check if it's an Asset or a Resource:
								Reference<Asset> asset = component->Context()->AssetDB()->FindAsset(objectId);
								if (asset != nullptr) {
									if (valueType.CheckType(asset))
										return setValue(asset);
									Reference<Resource> resource = asset->LoadResource();
									if (valueType.CheckType(resource))
										return setValue(resource);
								}
							}
							return setValue(nullptr);
						})) m_owner->SceneContext()->Log()->Error(
							"SceneUndoManager::UndoAction::RestoreSerializedData - Failed to restore data! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
				}
			}

			inline void RestoreReferencingObjects() {
				for (size_t i = 0; i < m_changes.size(); i++) {
					const ComponentDataChange& change = m_changes[i];
					m_owner->UpdateReferencingObjects(change.newData, change.oldData);
				}
			}

		protected:
			inline virtual void Undo() final override {
				const Reference<const ComponentSerializer::Set> serializers = ComponentSerializer::Set::All();
				std::unique_lock<std::recursive_mutex> lock(m_owner->SceneContext()->UpdateLock());
				RemoveCreatedComponents();
				CreateDeletedComponents(serializers);
				RestoreParentChildRelations();
				RestoreSerializedData(serializers);
				RestoreReferencingObjects();
				m_owner->SceneContext()->Log()->Error("SceneUndoManager::UndoAction::Undo - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}
		};

		SceneUndoManager::SceneUndoManager(Scene::LogicContext* context) : m_context(context) {
			assert(m_context != nullptr);
			TrackComponent(context->RootObject(), true);
			Flush();
		}

		SceneUndoManager::~SceneUndoManager() {}

		Scene::LogicContext* SceneUndoManager::SceneContext()const { return m_context; }

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
				UpdateReferencingObjects(change.oldData, change.newData);
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
			SceneContext()->Log()->Info("Recorded: ", changes.size());
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
				if (change.oldData == nullptr) {
					if (change.newData == nullptr) 
						return ComponentDataChange();
					else m_componentStates[guid] = change.newData;
				}
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
						GUID id = {};

						// We can have another Component:
						{
							Component* referencedComponent = dynamic_cast<Component*>(currentObject.operator->());
							if (CanTrackComponent(referencedComponent, SceneContext()) || (m_componentIds.find(referencedComponent) != m_componentIds.end())) {
								if (referencedComponent->Destroyed())
									addrSerializer->SetObjectValue(nullptr, addr.TargetAddr());
								else {
									id = GetGuid(referencedComponent);
									assert(id != (GUID{}));
								}
								if (id != guid) change.newData->referencedObjects.insert(id);
							}
						}

						// We may reference a resource:
						{
							Resource* resource = dynamic_cast<Resource*>(currentObject.operator->());
							if (resource != nullptr && resource->HasAsset())
								id = resource->GetAsset()->Guid();
						}

						// We may reference an asset:
						{
							Asset* asset = dynamic_cast<Asset*>(currentObject.operator->());
							if (asset != nullptr) id = asset->Guid();
						}

						// No matter what, we serialize the generated GUID:
						return Serialization::SerializeToJson(GUID_SERIALIZER->Serialize(id), SceneContext()->Log(), err, errorOnPointerSerializer);
					});
				if (error)
					SceneContext()->Log()->Error("SceneUndoManager::CreateComponentData - Component Snapshot created with errors! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			return change;
		}

		void SceneUndoManager::UpdateReferencingObjects(const ComponentData* oldData, const ComponentData* newData) {
			if (newData != nullptr)
				for (decltype(newData->referencedObjects)::const_iterator it = newData->referencedObjects.begin(); it != newData->referencedObjects.end(); ++it) {
					decltype(m_componentStates)::const_iterator stateIterator = m_componentStates.find(*it);
					if (stateIterator == m_componentStates.end()) continue;
					stateIterator->second->referencingObjects.insert(newData->guid);
				}
			if (oldData != nullptr)
				for (decltype(oldData->referencedObjects)::const_iterator it = oldData->referencedObjects.begin(); it != oldData->referencedObjects.end(); ++it) {
					decltype(m_componentStates)::const_iterator stateIterator = m_componentStates.find(*it);
					if (stateIterator == m_componentStates.end()) continue;
					if (newData == nullptr || newData->referencedObjects.find(*it) == newData->referencedObjects.end())
						stateIterator->second->referencingObjects.erase(oldData->guid);
				}
		}
	}
}

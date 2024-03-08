#include "Gizmo.h"


namespace Jimara {
	namespace Editor {
		GizmoScene::Context* Gizmo::GizmoContext()const {
			if (m_context == nullptr) 
				m_context = GizmoScene::GetContext(Context());
			return m_context;
		}

		Gizmo::~Gizmo() {}

		Reference<const Gizmo::ComponentConnectionSet> Gizmo::ComponentConnectionSet::Current() {
			// State:
			static SpinLock currentCollectionLock;
			static std::recursive_mutex connectionSetLock;
			static Reference<Gizmo::ComponentConnectionSet> currentSet;

			// Try get current set:
			Reference<Gizmo::ComponentConnectionSet> set;
			auto tryGetCurrent = [&]() -> bool {
				std::unique_lock<SpinLock> setLock(currentCollectionLock);
				set = currentSet;
				return (set != nullptr);
			};
			if (tryGetCurrent())
				return set;

			// Subscribe to OnRegisteredTypeSetChanged:
			struct RegistrySubscription : public virtual Object {
				inline static void OnRegisteredTypeSetChanged() {
					std::unique_lock<std::recursive_mutex> creationLock(connectionSetLock);
					std::unique_lock<SpinLock> lock(currentCollectionLock);
					currentSet = nullptr;
				}
				inline RegistrySubscription() {
					TypeId::OnRegisteredTypeSetChanged() += Callback(RegistrySubscription::OnRegisteredTypeSetChanged);
				}
				inline virtual ~RegistrySubscription() {
					TypeId::OnRegisteredTypeSetChanged() -= Callback(RegistrySubscription::OnRegisteredTypeSetChanged);
				}
			};
			static Reference<RegistrySubscription> registrySubscription;

			std::unique_lock<std::recursive_mutex> lock(connectionSetLock);
			if (tryGetCurrent())
				return set;

			if (registrySubscription == nullptr)
				registrySubscription = Object::Instantiate<RegistrySubscription>();

			// Collect registered connections:
			std::unordered_map<TypeId, std::unordered_map<TypeId, Reference<const Gizmo::ComponentConnection>>> gizmoConnections;
			const Reference<RegisteredTypeSet> currentTypes = RegisteredTypeSet::Current();
			for (size_t i = 0; i < currentTypes->Size(); i++) {
				auto checkAttribute = [&](const Object* attribute) {
					const ComponentConnection* connection = dynamic_cast<const ComponentConnection*>(attribute);
					if (connection == nullptr)
						return;
					if (connection->GizmoType() == TypeId::Of<void>())
						return;
					gizmoConnections[connection->GizmoType()][connection->ComponentType()] = connection;
				};
				currentTypes->At(i).GetAttributes(Callback<const Object*>::FromCall(&checkAttribute));
			}

			// Create new set:
			set = Object::Instantiate<ComponentConnectionSet>();
			for (auto& gizmoEntries : gizmoConnections)
				for (auto& componentEntry : gizmoEntries.second) {
					const ComponentConnection* connection = componentEntry.second;
					if (connection->ComponentType() != TypeId::Of<void>())
						set->m_connections[connection->ComponentType().TypeIndex()].Push(connection);
					if ((connection->FilterFlags() & FilterFlag::CREATE_WITHOUT_TARGET) != 0)
						set->m_targetlessGizmos.Push(connection);
				}

			// Update shared set reference:
			{
				std::unique_lock<SpinLock> setLock(currentCollectionLock);
				currentSet = set;
			}
			return set;
		}

		const Gizmo::ComponentConnectionSet::ConnectionList& Gizmo::ComponentConnectionSet::GetGizmosFor(std::type_index componentType)const {
			static const ConnectionList empty;
			auto it = m_connections.find(componentType);
			if (it == m_connections.end()) return empty;
			else return it->second;
		}
	}
}

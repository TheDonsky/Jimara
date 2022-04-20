#include "Gizmo.h"


namespace Jimara {
	namespace Editor {
		namespace {
			std::mutex gizmoConnectionLock;
			std::unordered_map<TypeId, std::unordered_map<TypeId, Gizmo::ComponentConnection>> gizmoConnections;
			Reference<Gizmo::ComponentConnectionSet> currentSet;
		}

		void Gizmo::AddConnection(const ComponentConnection& connection) {
			if (connection.GizmoType() == TypeId::Of<void>() || connection.ComponentType() == TypeId::Of<void>()) return;
			std::unique_lock<std::mutex> lock(gizmoConnectionLock);
			gizmoConnections[connection.GizmoType()][connection.ComponentType()] = connection;
			currentSet = nullptr;
		}

		void Gizmo::RemoveConnection(const ComponentConnection& connection) {
			if (connection.GizmoType() == TypeId::Of<void>() || connection.ComponentType() == TypeId::Of<void>()) return;
			std::unique_lock<std::mutex> lock(gizmoConnectionLock);
			
			auto gizmoIt = gizmoConnections.find(connection.GizmoType());
			if (gizmoIt == gizmoConnections.end()) return;
			
			auto componentIt = gizmoIt->second.find(connection.ComponentType());
			if (componentIt == gizmoIt->second.end()) return;
			
			gizmoIt->second.erase(componentIt);
			if (gizmoIt->second.empty())
				gizmoConnections.erase(gizmoIt);

			currentSet = nullptr;
		}

		Reference<const Gizmo::ComponentConnectionSet> Gizmo::ComponentConnectionSet::Current() {
			std::unique_lock<std::mutex> lock(gizmoConnectionLock);
			Reference<const ComponentConnectionSet> rv = currentSet;
			if (rv == nullptr) {
				rv = currentSet = Object::Instantiate<ComponentConnectionSet>();
				for (auto gizmoEntries : gizmoConnections)
					for (auto componentEntry : gizmoEntries.second) {
						const ComponentConnection connection = componentEntry.second;
						currentSet->m_connections[connection.ComponentType().TypeIndex()].Push(connection);
					}
			}
			return rv;
		}

		const Gizmo::ComponentConnectionSet::ConnectionList& Gizmo::ComponentConnectionSet::GetGizmosFor(std::type_index componentType)const {
			static const ConnectionList empty;
			auto it = m_connections.find(componentType);
			if (it == m_connections.end()) return empty;
			else return it->second;
		}
	}
}

#include "FBXObjectIndex.h"


namespace Jimara {
	namespace FBXHelpers {
		bool FBXObjectNode::Extract(const FBXContent::Node& objectNode, OS::Logger* logger) {
			const std::string_view nodeAttribute = objectNode.Name();

			if (objectNode.PropertyCount() < 3) {
				if (logger != nullptr) logger->Warning("FBXObjectNode::Extract - Node has less than 3 properties!");
				return false;
			}
			
			FBXUid uid = 0;
			if (!objectNode.NodeProperty(0).Get(uid)) {
				if (logger != nullptr) logger->Warning("FBXObjectNode::Extract - Node does not have a valid UID!");
				return false;
			}
			else if (uid == 0) {
				if (logger != nullptr) logger->Warning("FBXObjectNode::Extract - Node UID read as 0; This is a reserved value for the 'RootObject' and can not be used in a content node!");
				return false;
			}

			std::string_view nameClass;
			if (!objectNode.NodeProperty(1).Get(nameClass)) {
				if (logger != nullptr) logger->Warning("FBXObjectNode::Extract - Node does not have a 'Name::Class' string as it's second property!");
				return false;
			}
			
			const std::string_view name = nameClass.data();
			if ((name.size() + 2) > nameClass.size()) {
				if (logger != nullptr) logger->Warning("FBXObjectNode::Extract - Name::Class property too short!");
				return false;
			}
			else if (nameClass.data()[name.size()] != 0x00 || nameClass.data()[name.size() + 1] != 0x01) {
				if (logger != nullptr) logger->Warning("FBXObjectNode::Extract - Name::Class not formatted correctly!");
				return false;
			}
			const std::string_view classView(nameClass.data() + name.size() + 2, nameClass.size() - 2 - name.size());

			std::string_view subClass;
			if (!objectNode.NodeProperty(2).Get(subClass)) {
				if (logger != nullptr) logger->Warning("FBXObjectNode::Extract - Node does not have a 'Sub-Class' string as it's third property!");
				return false;
			}

			m_objectNode = &objectNode;
			m_uid = uid;
			m_nodeAttribute = nodeAttribute;
			m_nameClass = nameClass;
			m_name = name;
			m_class = classView;
			m_subClass = subClass;
			return true;
		}



		bool FBXObjectIndex::Build(const FBXContent::Node& rootNode, OS::Logger* logger) {
			Clear();
			if (!CollectObjectNodes(rootNode, logger)) return false;
			else if (!BuildConnectionIndex(rootNode, logger)) return false;
			else return true;
		}


		void FBXObjectIndex::Clear() {
			m_nodes.clear();
			m_nodeIndexByUid.clear();
		}
		
		bool FBXObjectIndex::CollectObjectNodes(const FBXContent::Node& rootNode, OS::Logger* logger) {
			const FBXContent::Node* objectsNode = rootNode.FindChildNodeByName("Objects", 8);
			if (objectsNode == nullptr) return true;
			for (size_t i = 0; i < objectsNode->NestedNodeCount(); i++) {
				FBXObjectNode node;
				if (!node.Extract(objectsNode->NestedNode(i), logger)) {
					if (logger != nullptr) logger->Warning("FBXObjectIndex::CollectObjectNodes - Objects[", i, "] header not formatted as a valid Object node and will be ignored...");
					continue;
				}
				if (m_nodeIndexByUid.find(node.Uid()) != m_nodeIndexByUid.end()) {
					if (logger != nullptr) logger->Error("FBXObjectIndex::CollectObjectNodes - Objects[", i, "] has a duplicate UID; Our assumption is for the UID-s to be unique!");
					return false;
				}
				m_nodeIndexByUid[node.Uid()] = m_nodes.size();
				m_nodes.push_back(node);
			}
			return true;
		}

		bool FBXObjectIndex::BuildConnectionIndex(const FBXContent::Node& rootNode, OS::Logger* logger) {
			const FBXContent::Node* connectionsNode = rootNode.FindChildNodeByName("Connections", 9);
			if (connectionsNode == nullptr) return true;
			for (size_t nestedNodeId = 0; nestedNodeId < connectionsNode->NestedNodeCount(); nestedNodeId++) {
				const FBXContent::Node& connectionNode = connectionsNode->NestedNode(nestedNodeId);
				if (connectionNode.Name() != "C") continue;
				else if (connectionNode.PropertyCount() < 3) {
					if (logger != nullptr) logger->Warning("FBXObjectIndex::BuildConnectionIndex - Connection node incomplete!");
					continue;
				}
				
				std::string_view connectionType;
				if (!connectionNode.NodeProperty(0).Get(connectionType)) {
					if (logger != nullptr) logger->Error("FBXObjectIndex::BuildConnectionIndex - Connection node does not have a valid connection type string!");
					return false;
				}
				FBXUid childUid, parentUid;
				if (!connectionNode.NodeProperty(1).Get(childUid)) {
					if (logger != nullptr) logger->Error("FBXData::Extract - Connection node child UID invalid!");
					return false;
				}
				if (!connectionNode.NodeProperty(2).Get(parentUid)) {
					if (logger != nullptr) logger->Error("FBXData::Extract - Connection node parent UID invalid!");
					return false;
				}

				if (childUid == 0 || parentUid == 0) continue;
				const NodeIndexByUid::const_iterator childIt = m_nodeIndexByUid.find(childUid);
				if (childIt == m_nodeIndexByUid.end()) {
					if (logger != nullptr) logger->Warning("FBXObjectIndex::BuildConnectionIndex - Child UID not pointing to a valid Object node!");
					continue;
				}
				const NodeIndexByUid::const_iterator parentIt = m_nodeIndexByUid.find(parentUid);
				if (parentIt == m_nodeIndexByUid.end()) {
					if (logger != nullptr) logger->Warning("FBXObjectIndex::BuildConnectionIndex - Parent UID not pointing to a valid Object node!");
					continue;
				}

				ObjectPropertyId parentConnection(m_nodes.data() + parentIt->second);
				if (connectionNode.PropertyCount() > 3) {
					std::string_view propertyView;
					if (!connectionNode.NodeProperty(3).Get(propertyView)) {
						if (logger != nullptr) logger->Error("FBXData::Extract - Linked property name entry present, but is not a string!");
						return false;
					}
					else parentConnection.propertyName = propertyView;
				}

				NodeWithConnections& childNode = m_nodes[childIt->second];
				bool connectionUnique = true;
				for (size_t i = 0; i < childNode.parentConnections.Size(); i++)
					if (childNode.parentConnections[i] == parentConnection) {
						//if (logger != nullptr) logger->Warning("FBXObjectIndex::BuildConnectionIndex - Found a duplicate connection!");
						connectionUnique = false;
						break;
					}
				if (connectionUnique) {
					childNode.parentConnections.Push(parentConnection);
					m_nodes[parentIt->second].childConnections.Push(ObjectPropertyId(m_nodes.data() + childIt->second, parentConnection.propertyName));
				}
			}
			return true;
		}
	}
}

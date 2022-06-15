#pragma once
#include "FBXContent.h"
#include "FBXObjects.h"
#include <unordered_map>
#include <optional>


namespace Jimara {
	namespace FBXHelpers {
		/// <summary>
		/// Command data for general node parsing
		/// Note: Underlying node's lifecycle is not tied to these objects and pretending that it does will more than likely result in a disaster
		/// </summary>
		class JIMARA_API FBXObjectNode {
		public:
			/// <summary>
			/// Attempts to extract basic node information about the FBX node.
			/// Note: This one attempts to extract the fields, but does no analisis about the classes, uniqueness or whatever.
			/// </summary>
			/// <param name="objectNode"> Parsed FBX node that, presumably, contains Object data </param>
			/// <param name="logger"> Logger for warning reporting </param>
			/// <returns> true, if the object basics were successfully extracted from the node </returns>
			bool Extract(const FBXContent::Node& objectNode, OS::Logger* logger);

			/// <summary> Node, containing object data </summary>
			inline const FBXContent::Node* Node()const { return m_objectNode; }

			/// <summary> NodeAttribute/TypeIdentifier for the object (for example, "Geometry" for meshes) </summary>
			inline const std::string_view& NodeAttribute()const { return m_nodeAttribute; }

			/// <summary> Inique object identifier (general assumption is that this should be unique for each object from the same file, independent of the type) </summary>
			inline const FBXUid Uid()const { return m_uid; }

			/// <summary> 
			/// Object name, followed by b"\x00\x01", followed by "Class" (reffered as "Name::Class")
			/// Note: Sometimes same as NodeAttribute, but not always; each NodeAttribute has it's own values and individual parsers are responsible for handling those.
			/// </summary>
			inline const std::string_view& NameClass()const { return m_nameClass; }

			/// <summary> "Name" from "Name::Class" </summary>
			inline const std::string_view& Name()const { return m_name; }

			/// <summary> "Class" from "Name::Class" </summary>
			inline const std::string_view& Class()const { return m_class; }

			/// <summary> "Sub-Class" field (Based on NodeAttribute, this mmay be interpreted in various ways, probably) </summary>
			inline const std::string_view& SubClass()const { return m_subClass; }

		private:
			// Node, containing object data
			const FBXContent::Node* m_objectNode = nullptr;
			
			// Unique identifier
			FBXUid m_uid = 0;

			// NodeAttribute, Name::Class, Name(from Name::Class), Class(from Name::Class) and Sub-Class
			std::string_view m_nodeAttribute, m_nameClass, m_name, m_class, m_subClass;
		};


		/// <summary>
		/// Utility that gathers the Objects and Connections from FBXContent and attempts to make sense of it all
		/// </summary>
		class JIMARA_API FBXObjectIndex {
		public:
			struct NodeWithConnections;

			/// <summary>
			/// Connection information
			/// </summary>
			struct ObjectPropertyId {
				/// <summary> "Other" end of the connection </summary>
				const NodeWithConnections* connection;

				/// <summary> "Connection Property Name", where applicable (for materials, mostly... probably... I don't care) </summary>
				std::optional<std::string_view> propertyName;

				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="object"> Connected object </param>
				/// <param name="propName"> [Optional] Property name </param>
				inline ObjectPropertyId(const NodeWithConnections* object = 0, const std::optional<std::string_view>& propName = std::optional<std::string_view>())
					: connection(object), propertyName(propName) {};

				/// <summary>
				/// Checks if the two connections are the same (don't think about this)
				/// </summary>
				/// <param name="other"> Connection to compare to </param>
				/// <returns> True, if the connection and propertyName match </returns>
				inline bool operator==(const ObjectPropertyId& other)const {
					return (connection == other.connection) &&
						(propertyName.has_value() == other.propertyName.has_value()) &&
						((!propertyName.has_value()) || (propertyName.value() == other.propertyName.value()));
				}
			};

			/// <summary>
			/// FBX node, alongside it's connections, both parent and child, because I said so...
			/// </summary>
			struct JIMARA_API NodeWithConnections {
				/// <summary> Node </summary>
				FBXObjectNode node;

				/// <summary> "Parents"/"Users" of the node (for the dummy me down the line, something like a Geometry node will have this filled with corresponding transform) </summary>
				Stacktor<ObjectPropertyId, 8> parentConnections;

				/// <summary> "Child"/"Used" nodes (basically, reverse of whatever the parentConnections stores, but on the other side) </summary>
				Stacktor<ObjectPropertyId, 16> childConnections;
				inline NodeWithConnections(const FBXObjectNode& n = FBXObjectNode()) : node(n) {}
			};


			/// <summary>
			/// Builds connection data form FBXContent
			/// </summary>
			/// <param name="rootNode"> FBXContent::RootNode() </param>
			/// <param name="logger"> Logger for error/warning reporting </param>
			/// <returns> True, if no error occures </returns>
			bool Build(const FBXContent::Node& rootNode, OS::Logger* logger);

			/// <summary> Number of nodes extracted </summary>
			inline size_t ObjectCount()const { return m_nodes.size(); }

			/// <summary>
			/// Object node by index
			/// </summary>
			/// <param name="index"> Object node's index [valid range is [0-ObjectCount())] </param>
			/// <returns> Node data </returns>
			inline const NodeWithConnections& ObjectNode(size_t index)const { return m_nodes[index]; }

			/// <summary>
			/// Attempts to find node by UID
			/// </summary>
			/// <param name="uid"> Unique identifier from FBX file </param>
			/// <returns> Index for ObjectNode() if successful, no value otherwise </returns>
			inline std::optional<size_t> ObjectNodeByUID(FBXUid uid)const {
				const NodeIndexByUid::const_iterator it = m_nodeIndexByUid.find(uid);
				return (it == m_nodeIndexByUid.end()) ? std::optional<size_t>() : std::optional<size_t>(it->second);
			}


		private:
			// Node data
			std::vector<NodeWithConnections> m_nodes;
			
			// Node index
			typedef std::unordered_map<FBXUid, size_t> NodeIndexByUid;
			NodeIndexByUid m_nodeIndexByUid;

			// Basic data collection steps in order:

			// We need to clear the underlying collections first;
			void Clear();

			// Next step, we extract nodes without connections;
			bool CollectObjectNodes(const FBXContent::Node& rootNode, OS::Logger* logger);

			// Lastly, we need to add the collections in, while making sure each connection is valid.
			bool BuildConnectionIndex(const FBXContent::Node& rootNode, OS::Logger* logger);
		};
	}
}

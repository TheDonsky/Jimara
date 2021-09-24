#include "FBXData.h"


namespace Jimara {
	Reference<FBXData> FBXData::Extract(const FBXContent* sourceContent, OS::Logger* logger) {
		auto error = [&](auto... message) {
			if (logger != nullptr) logger->Error(message...);
			return nullptr;
		};

		auto warning = [&](auto... message) {  if (logger != nullptr) logger->Warning(message...); };

		if (sourceContent == nullptr) return error("FBXData::Extract - NULL sourceContent provided!");

		// Just a handy utility to find child nodes by name:
		auto findChildNode = [](const FBXContent::Node* parentNode, const std::string_view& childName, size_t expectedIndex = 0) -> const FBXContent::Node* {
			if (parentNode == nullptr) return nullptr;
			size_t nestedNodeCount = parentNode->NestedNodeCount();
			if (nestedNodeCount <= 0) return nullptr;
			expectedIndex %= nestedNodeCount;
			size_t i = expectedIndex;
			while (true) {
				const FBXContent::Node* childNode = &parentNode->NestedNode(i);
				if (childNode->Name() == childName) return childNode;
				i++;
				if (i == expectedIndex) return nullptr;
				else if (i >= nestedNodeCount) i -= nestedNodeCount;
			}
		};

		auto findNodeChild = [&](const FBXContent::Node* parentNode, const std::string_view& childName, size_t expectedIndex, bool warnIfMissing, auto... warningMessage) {
			const FBXContent::Node* node = findChildNode(parentNode, childName, expectedIndex);
			if (node == nullptr) warning(warningMessage...);
			return node;
		};

		// Root level nodes:
		const FBXContent::Node* fbxHeaderExtensionNode = findNodeChild(&sourceContent->RootNode(), "FBXHeaderExtension", 0, true, "FBXData::Extract - FBXHeaderExtension missing!");
		const FBXContent::Node* fileIdNode = findNodeChild(&sourceContent->RootNode(), "FileId", 1, true, "FBXData::Extract - FileId missing!");
		const FBXContent::Node* creationTimeNode = findNodeChild(&sourceContent->RootNode(), "CreationTime", 2, true, "FBXData::Extract - CreationTime missing!");
		const FBXContent::Node* creatorNode = findNodeChild(&sourceContent->RootNode(), "Creator", 3, true, "FBXData::Extract - Creator missing!");
		const FBXContent::Node* globalSettingsNode = findNodeChild(&sourceContent->RootNode(), "GlobalSettings", 4, true, "FBXData::Extract - GlobalSettings missing!");
		const FBXContent::Node* documentsNode = findChildNode(&sourceContent->RootNode(), "Documents", 5);
		const FBXContent::Node* referencesNode = findChildNode(&sourceContent->RootNode(), "References", 6);
		const FBXContent::Node* definitionsNode = findChildNode(&sourceContent->RootNode(), "Definitions", 7);
		const FBXContent::Node* objectsNode = findChildNode(&sourceContent->RootNode(), "Objects", 8);
		const FBXContent::Node* connectionsNode = findChildNode(&sourceContent->RootNode(), "Connections", 9);
		const FBXContent::Node* takesNode = findChildNode(&sourceContent->RootNode(), "Takes", 10);

		// Notes:
		// 0. We ignore the contents of FBXHeaderExtension for performance and also, we don't really care for them; invalid FBX files may pass here;
		// 1. We also don't care about FileId, CreationTime Creator, Documents, References and Takes for similar reasons;
		// 2. I've followed this link to make sense of the content: 
		//    https://web.archive.org/web/20160605023014/https://wiki.blender.org/index.php/User:Mont29/Foundation/FBX_File_Structure#Spaces_.26_Parenting
		Reference<FBXData> result = Object::Instantiate<FBXData>();

		// __TODO__: Parse Definitions...

		// __TODO__: Parse Objects...
		if (objectsNode != nullptr) {
			// Reads a Mesh:
			auto readMesh = [&](const FBXContent::Node* meshNode) -> bool {
				// __TODO__: Implement this crap!

				return true;
			};


			for (size_t i = 0; i < objectsNode->NestedNodeCount(); i++) {
				const FBXContent::Node* objectNode = &objectsNode->NestedNode(i);

				// NodeAttribute:
				const std::string_view& nodeAttribute = objectNode->Name();

				// Property count:
				if (objectNode->PropertyCount() < 3) {
					warning("FBXData::Extract - Object[", i, "] has less than 3 properties. Expected [UID, Name::Class, Sub-Class]; Object entry will be ignored...");
					continue;
				}

				// UID:
				const FBXContent::Property& objectUidProperty = objectNode->NodeProperty(0);
				int64_t objectUid;
				if (objectUidProperty.Type() == FBXContent::PropertyType::INT_64) objectUid = ((int64_t)objectUidProperty);
				else if (objectUidProperty.Type() == FBXContent::PropertyType::INT_32) objectUid = static_cast<int64_t>((int32_t)objectUidProperty);
				else if (objectUidProperty.Type() == FBXContent::PropertyType::INT_16) objectUid = static_cast<int64_t>((int16_t)objectUidProperty);
				else {
					warning("FBXData::Extract - Object[", i, "].NodeProperty[0]<UID> is not an integer type; Object entry will be ignored...");
					continue;
				}

				// Name::Class
				const FBXContent::Property& nameClassProperty = objectNode->NodeProperty(1);
				if (nameClassProperty.Type() != FBXContent::PropertyType::STRING) {
					warning("FBXData::Extract - Object[", i, "].NodeProperty[1]<Name::Class> is not a string; Object entry will be ignored...");
					continue;
				}

				// Sub-class:
				const FBXContent::Property& subClassProperty = objectNode->NodeProperty(2);
				if (subClassProperty.Type() != FBXContent::PropertyType::STRING) {
					warning("FBXData::Extract - Object[", i, "].NodeProperty[2]<Sub-class> is not a string; Object entry will be ignored...");
					continue;
				}

				// Distinguish object types:
				if (nodeAttribute == "Model") {
					warning("FBXData::Extract - Object[", i, "].Name() = '", nodeAttribute, "'; [__TODO__]: Parser not yet implemented! Object entry will be ignored...");
					continue;
				}
				else if (nodeAttribute == "Light") { // Not sure this is the name... Maybe, we need to change this one...
					warning("FBXData::Extract - Object[", i, "].Name() = '", nodeAttribute, "'; [__TODO__]: Parser not yet implemented! Object entry will be ignored...");
					continue;
				}
				else if (nodeAttribute == "Camera") { // Not sure this is the name... Maybe, we need to change this one...
					warning("FBXData::Extract - Object[", i, "].Name() = '", nodeAttribute, "'; [__TODO__]: Parser not yet implemented! Object entry will be ignored...");
					continue;
				}
				else if (nodeAttribute == "Geometry") {
					if (!readMesh(objectNode)) return nullptr;
				}
				else {
					warning("FBXData::Extract - Object[", i, "].Name() = '", nodeAttribute, "': Unknown type! Object entry will be ignored...");
					continue;
				}
			}
		}

		// __TODO__: Parse Connections...

		return result;
	}
}

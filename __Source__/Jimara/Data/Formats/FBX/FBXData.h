#pragma once
#include "FBXContent.h"
#include "../../Mesh.h"
#include <unordered_map>


namespace Jimara {
	class FBXData : public virtual Object {
	public:
		static Reference<FBXData> Extract(const FBXContent* sourceContent, OS::Logger* logger);

		struct FBXGlobalSettings {
			Vector3 forwardAxis = Math::Forward();
			Vector3 upAxis = Math::Up();
			Vector3 coordAxis = Math::Right();
			float unitScale = 1.0f;
		};

		typedef int64_t ObjectId;

		struct FBXObject {
			ObjectId objectId = 0;
		};

		struct FBXMesh : public FBXObject {
			Reference<const PolyMesh> mesh;
		};

		struct FBXNode : public FBXObject {
			Vector3 position = Vector3(0.0f);
			Vector3 rotation = Vector3(0.0f);
			Vector3 scale = Vector3(1.0f);

			Stacktor<size_t, 1> meshIndices;
			std::vector<FBXNode> children;
		};

		const FBXGlobalSettings& Settings()const;

		size_t MeshCount()const;

		const FBXMesh& GetMesh(size_t index)const;

		const FBXNode& RootNode()const;

	private:
		FBXGlobalSettings m_globalSettings;
		FBXNode m_rootNode;
		std::vector<FBXMesh> m_meshes;
		std::unordered_map<ObjectId, size_t> m_meshIndex;
	};
}

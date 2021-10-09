#pragma once
#include "FBXContent.h"
#include "FBXObjects.h"
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

		const FBXGlobalSettings& Settings()const;

		size_t MeshCount()const;

		const FBXMesh* GetMesh(size_t index)const;

		const FBXNode* RootNode()const;

	private:
		FBXGlobalSettings m_globalSettings;
		Reference<FBXNode> m_rootNode = Object::Instantiate<FBXNode>();
		std::vector<Reference<FBXMesh>> m_meshes;
	};
}

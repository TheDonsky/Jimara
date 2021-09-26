#pragma once
#include "FBXContent.h"
#include "../../Mesh.h"
#include <unordered_map>


namespace Jimara {
	class FBXData : public virtual Object {
	public:
		static Reference<FBXData> Extract(const FBXContent* sourceContent, OS::Logger* logger);

		typedef int64_t ObjectId;

		struct FBXObject {
			ObjectId objectId = 0;
		};

		struct FBXMesh : public FBXObject {
			Reference<const PolyMesh> polygonalMesh;
			Reference<const TriMesh> triangleMesh;
		};

	private:
		std::vector<FBXMesh> m_meshes;
		std::unordered_map<ObjectId, size_t> m_meshIndex;
	};
}

#pragma once
#include "../../Mesh.h"
#include <cstdint>

namespace Jimara {
	// Type definition for UID
	typedef int64_t FBXUid;

	/// <summary> Object from an FBX file </summary>
	struct FBXObject : public virtual Object {
		/// <summary> UID form FBX file </summary>
		FBXUid uid = 0;
	};

	/// <summary> Mesh data from an FBX file </summary>
	struct FBXMesh : public FBXObject {
		/// <summary> Mesh from an FBX file </summary>
		Reference<const PolyMesh> mesh;
	};

	/// <summary> Transform, alongside the attached renderers and what not from an FBX file </summary>
	struct FBXNode : public FBXObject {
		/// <summary> Name of the object </summary>
		std::string name = "";

		/// <summary> Local position </summary>
		Vector3 position = Vector3(0.0f);

		/// <summary> Local euler angles </summary>
		Vector3 rotation = Vector3(0.0f);

		/// <summary> Local scale </summary>
		Vector3 scale = Vector3(1.0f);

		/// <summary> Attached geometry </summary>
		Stacktor<Reference<const FBXMesh>, 1> meshes;

		/// <summary> Child nodes </summary>
		std::vector<Reference<const FBXNode>> children;
	};
}

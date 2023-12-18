#pragma once
#include "../../Geometry/Mesh.h"
#include "../../Animation.h"
#include <cstdint>
#include <optional>


namespace Jimara {
	// Type definition for UID
	typedef int64_t FBXUid;

	/// <summary> Object from an FBX file </summary>
	struct JIMARA_API FBXObject : public Object {
		/// <summary> UID form FBX file </summary>
		FBXUid uid = 0;

		/// <summary> Constructor </summary>
		inline FBXObject() {}

		/// <summary>
		/// Copy-Constructor
		/// </summary>
		/// <param name="other"> Source </param>
		inline FBXObject(const FBXObject& other) : uid(other.uid) {}

		/// <summary>
		/// Copy-Assignment
		/// </summary>
		/// <param name="other"> Source </param>
		/// <returns> self </returns>
		inline FBXObject& operator=(const FBXObject& other) {
			uid = other.uid;
			return (*this);
		}
	};

	/// <summary> Mesh data from an FBX file </summary>
	struct JIMARA_API FBXMesh : public FBXObject {
		/// <summary> Mesh from an FBX file </summary>
		Reference<PolyMesh> mesh;
	};

	/// <summary> Skinned Mesh data from an FBX file </summary>
	struct JIMARA_API FBXSkinnedMesh : public FBXMesh {
		std::optional<FBXUid> rootBoneId = 0;
		std::vector<FBXUid> boneIds;

		inline SkinnedPolyMesh* SkinnedMesh()const { return dynamic_cast<SkinnedPolyMesh*>(mesh.operator->()); }
	};

	/// <summary> Transform, alongside the attached renderers and what not from an FBX file </summary>
	struct JIMARA_API FBXNode : public FBXObject {
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

	/// <summary> Animation, extracted from FBX </summary>
	struct JIMARA_API FBXAnimation : public FBXObject {
		/// <summary> Animation clip </summary>
		Reference<AnimationClip> clip;
	};
}

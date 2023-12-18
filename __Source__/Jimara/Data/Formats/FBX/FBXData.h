#pragma once
#include "FBXContent.h"
#include "FBXObjects.h"
#include "../../Geometry/Mesh.h"
#include <unordered_map>


namespace Jimara {
	/// <summary>
	/// Actual data, stored inside an FBX file
	/// </summary>
	class JIMARA_API FBXData : public virtual Object {
	public:
		/// <summary>
		/// Extracts FBX Data from an FBX Content
		/// </summary>
		/// <param name="sourceContent"> Content from an FBX file </param>
		/// <param name="logger"> Logger for error reporting </param>
		/// <returns> Extracted data if successful, nullptr if something went wrong </returns>
		static Reference<FBXData> Extract(const FBXContent* sourceContent, OS::Logger* logger);

		/// <summary>
		/// Extracts FBX Data from a memory block
		/// </summary>
		/// <param name="block"> Memory block (could be something like a memory-mapped FBX file) </param>
		/// <param name="logger"> Logger for error reporting </param>
		/// <returns> Extracted data if successful, nullptr if something went wrong </returns>
		static Reference<FBXData> Extract(const MemoryBlock& block, OS::Logger* logger);

		/// <summary>
		/// Extracts FBX Data from an FBX file
		/// </summary>
		/// <param name="sourcePath"> FBX file path </param>
		/// <param name="logger"> Logger for error reporting </param>
		/// <returns> Extracted data if successful, nullptr if something went wrong </returns>
		static Reference<FBXData> Extract(const OS::Path& sourcePath, OS::Logger* logger);

		/// <summary>
		/// General report of the global settings of the FBX file
		/// </summary>
		struct FBXGlobalSettings {
			Vector3 forwardAxis = Math::Forward();
			Vector3 upAxis = Math::Up();
			Vector3 coordAxis = Math::Right();
			float unitScale = 1.0f;
		};

		/// <summary> Global settings (information only; no need to interpret this guy in any way) </summary>
		const FBXGlobalSettings& Settings()const;

		/// <summary> Number of meshes, stored inside an FBX file </summary>
		size_t MeshCount()const;

		/// <summary>
		/// FBXMesh by index
		/// </summary>
		/// <param name="index"> Mesh index </param>
		/// <returns> Mesh by index </returns>
		const FBXMesh* GetMesh(size_t index)const;

		/// <summary> Number of animation clips, extracted from the FBX file </summary>
		size_t AnimationCount()const;

		/// <summary>
		/// FBXAnimation by index
		/// </summary>
		/// <param name="index"> Animation clip index </param>
		/// <returns> Animation clip by index </returns>
		const FBXAnimation* GetAnimation(size_t index)const;

		/// <summary> Root transform node </summary>
		const FBXNode* RootNode()const;
		

	private:
		// Global settings
		FBXGlobalSettings m_globalSettings;

		// Root transform node
		Reference<FBXNode> m_rootNode = Object::Instantiate<FBXNode>();
		
		// Meshes from the FBX file
		std::vector<Reference<FBXMesh>> m_meshes;

		// Animations from the FBX file
		std::vector<Reference<FBXAnimation>> m_animations;
	};
}

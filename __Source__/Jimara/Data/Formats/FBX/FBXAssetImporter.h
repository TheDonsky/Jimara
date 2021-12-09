#pragma once
#include "../../AssetDatabase/FileSystemDatabase/FileSystemDatabase.h"
#include "../../TypeRegistration/TypeRegistartion.h"
#include "FBXObjects.h"


namespace Jimara {
	namespace FBXHelpers {
		JIMARA_REGISTER_TYPE(Jimara::FBXHelpers::FBXAssetImporter);

		/// <summary>
		/// FileSystemDatabase::AssetImporter for fbx files
		/// Note: Just like everything else in Jimara::FBXHelpers, this one should be of no interest for the user; 
		///		FileSystemDatabase will "automagically" be able to utilize it's functionality.
		/// </summary>
		class FBXAssetImporter : public virtual FileSystemDatabase::AssetImporter {
		public:
			/// <summary>
			/// Imports assets from the file
			/// </summary>
			/// <param name="reportAsset"> Whenever the FileReader detects an asset within the file, it should report it's presence through this callback </param>
			/// <returns> True, if the entire file got parsed successfully, false otherwise </returns>
			virtual bool Import(Callback<Asset*> reportAsset) final override;

		private:
			// Each time the asset gets reimported, we bump this value to make sure old Assets from cache do not get reused
			std::atomic<size_t> m_revision = 0;

			// FBXUid to Asset GUID translation:
			typedef std::map<FBXUid, GUID> FBXUidToGUID;
			FBXUidToGUID m_polyMeshGUIDs;
			FBXUidToGUID m_triMeshGUIDs;
			FBXUidToGUID m_animationGUIDs;

			// We need Registration functions and they have to be accessible through BuiltInTypeRegistrator:
			JIMARA_DEFINE_TYPE_REGISTRATION_CALLBACKS;
			friend class ::Jimara::BuiltInTypeRegistrator;
		};
	}
}

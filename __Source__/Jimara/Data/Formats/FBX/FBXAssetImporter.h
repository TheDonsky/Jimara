#pragma once
#include "../../AssetDatabase/FileSystemDatabase/FileSystemDatabase.h"
#include "../../TypeRegistration/TypeRegistartion.h"
#include "FBXObjects.h"


namespace Jimara {
	namespace FBXHelpers {
		JIMARA_REGISTER_TYPE(Jimara::FBXHelpers::FBXAssetImporter);

		/// <summary>
		/// Registers FileSystemDatabase::AssetImporter for fbx files
		/// Note: Just like everything else in Jimara::FBXHelpers, this one should be of no interest for the user; 
		///		FileSystemDatabase will "automagically" be able to utilize it's functionality.
		/// </summary>
		class FBXAssetImporter {
		private:
			// Nobody's gonna create an instance of this
			inline FBXAssetImporter() = delete;
		};
	}

	// Type registration callbacks
	template<> void TypeIdDetails::OnRegisterType<FBXHelpers::FBXAssetImporter>();
	template<> void TypeIdDetails::OnUnregisterType<FBXHelpers::FBXAssetImporter>();
}

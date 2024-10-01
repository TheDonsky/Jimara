#pragma once
#include "../../AssetDatabase/FileSystemDatabase/FileSystemDatabase.h"
#include "../../../Core/TypeRegistration/TypeRegistration.h"
#include "FBXObjects.h"


namespace Jimara {
	namespace FBXHelpers {
		JIMARA_REGISTER_TYPE(Jimara::FBXHelpers::FBXAssetImporter);

		/// <summary>
		/// Registers FileSystemDatabase::AssetImporter for fbx files
		/// Note: Just like everything else in Jimara::FBXHelpers, this one should be of no interest for the user; 
		///		FileSystemDatabase will "automagically" be able to utilize it's functionality.
		/// </summary>
		class JIMARA_API FBXAssetImporter {
		private:
			// Nobody's gonna create an instance of this
			inline FBXAssetImporter() = delete;
		};
	}

	// Type registration callbacks
	template<> JIMARA_API void TypeIdDetails::OnRegisterType<FBXHelpers::FBXAssetImporter>();
	template<> JIMARA_API void TypeIdDetails::OnUnregisterType<FBXHelpers::FBXAssetImporter>();
}

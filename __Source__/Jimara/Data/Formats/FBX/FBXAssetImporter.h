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
			// We need Registration functions and they have to be accessible through BuiltInTypeRegistrator:
			JIMARA_DEFINE_TYPE_REGISTRATION_CALLBACKS;
			friend class ::Jimara::BuiltInTypeRegistrator;

			// Nobody's gonna create an instance of this
			inline FBXAssetImporter() = delete;
		};
	}
}

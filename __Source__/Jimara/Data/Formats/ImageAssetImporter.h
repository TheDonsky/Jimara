#pragma once
#include "../../Core/TypeRegistration/TypeRegistration.h"

namespace Jimara {
	/// <summary> Register image formats </summary>
	JIMARA_REGISTER_TYPE(ImageAssetImporter);

	/// <summary>
	/// Registers FileSystemDatabase::AssetImporter for image files
	/// Note: This one should be of no interest for the user; FileSystemDatabase will "automagically" be able to utilize it's functionality.
	/// </summary>
	class JIMARA_API ImageAssetImporter {
	private:
		// Nobody's gonna create an instance of this
		inline ImageAssetImporter() = delete;
	};

	// Type registration callbacks
	template<> void TypeIdDetails::OnRegisterType<ImageAssetImporter>();
	template<> void TypeIdDetails::OnUnregisterType<ImageAssetImporter>();
}

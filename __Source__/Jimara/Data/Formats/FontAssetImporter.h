#pragma once
#include "../../Core/TypeRegistration/TypeRegistartion.h"


namespace Jimara {
	/// <summary> Register font formats </summary>
	JIMARA_REGISTER_TYPE(FontAssetImporter);
	class JIMARA_API FontAssetImporter;

	/// <summary>
	/// Registers FileSystemDatabase::AssetImporter for font files
	/// Note: This one should be of no interest for the user; FileSystemDatabase will "automagically" be able to utilize it's functionality.
	/// </summary>
	class JIMARA_API FontAssetImporter {
	private:
		// Nobody's gonna create an instance of this
		inline FontAssetImporter() = delete;
	};

	// Type registration callbacks
	template<> void TypeIdDetails::OnRegisterType<FontAssetImporter>();
	template<> void TypeIdDetails::OnUnregisterType<FontAssetImporter>();
}

#pragma once
#include "../../Core/TypeRegistration/TypeRegistartion.h"

namespace Jimara {
	/// <summary> Register audio formats </summary>
	JIMARA_REGISTER_TYPE(AudioAssetImporter);

	/// <summary>
	/// Registers FileSystemDatabase::AssetImporter for audio files
	/// Note: This one should be of no interest for the user; FileSystemDatabase will "automagically" be able to utilize it's functionality.
	/// </summary>
	class JIMARA_API AudioAssetImporter {
	private:
		// Nobody's gonna create an instance of this
		inline AudioAssetImporter() = delete;
	};

	// Type registration callbacks
	template<> void TypeIdDetails::OnRegisterType<AudioAssetImporter>();
	template<> void TypeIdDetails::OnUnregisterType<AudioAssetImporter>();
}

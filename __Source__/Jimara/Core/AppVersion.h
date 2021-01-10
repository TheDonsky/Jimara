#pragma once


namespace Jimara {
	/// <summary> Basic application version descriptor </summary>
	struct AppVersion {
		/// <summary> Major, Minor and Patch define the application version </summary>
		int major, minor, patch;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="vMajor"> major </param>
		/// <param name="vMinor"> minor </param>
		/// <param name="vPatch"> patch </param>
		inline AppVersion(int vMajor = 0, int vMinor = 0, int vPatch = 0) : major(vMajor), minor(vMinor), patch(vPatch) { }
	};

	/// <summary> Name of the game engine </summary>
	static const char ENGINE_NAME[] = "Jimara";

	/// <summary> Game engine version </summary>
	static const AppVersion ENGINE_VERSION(0, 0, 0);
}

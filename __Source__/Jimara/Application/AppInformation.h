#pragma once
#include "../Core/Object.h"
#include "AppVersion.h"
#include <string>

namespace Jimara {
	namespace Application {
		/// <summary>
		/// Basic information about the application
		/// </summary>
		class AppInformation : public virtual Object {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="appName"> Application name </param>
			/// <param name="appVersion"> Application version </param>
			AppInformation(const std::string& appName, const AppVersion& appVersion);

			/// <summary> Application name </summary>
			const char* ApplicationName()const;

			/// <summary> Application version </summary>
			const AppVersion ApplicationVersion()const;

			/// <summary> Engine name </summary>
			static const char* EngineName();

			/// <summary> Engine build version </summary>
			static const AppVersion EngineVersion();

		private:
			// Application name
			const std::string m_appName;

			// Application version
			const AppVersion m_appVersion;
		};
	}
}

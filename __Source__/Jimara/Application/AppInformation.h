#pragma once
#include "../Core/Object.h"
#include "AppVersion.h"
#include <string_view>
#include <string>

namespace Jimara {
	namespace Application {
		/// <summary>
		/// Basic information about the application
		/// </summary>
		class JIMARA_API AppInformation : public virtual Object {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="appName"> Application name </param>
			/// <param name="appVersion"> Application version </param>
			AppInformation(const std::string_view& appName = EngineName(), const AppVersion& appVersion = EngineVersion());

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

#include "AppInformation.h"


namespace Jimara {
	namespace Application {
		AppInformation::AppInformation(const std::string& appName, const AppVersion& appVersion) 
			: m_appName(appName), m_appVersion(appVersion) {}

		const char* AppInformation::ApplicationName()const {
			return m_appName.c_str();
		}

		const AppVersion AppInformation::ApplicationVersion()const {
			return m_appVersion;
		}

		const char* AppInformation::EngineName() {
			static const char ENGINE_NAME[] = "Jimara";
			return ENGINE_NAME;
		}

		const AppVersion AppInformation::EngineVersion() {
			return AppVersion(0, 0, 1);
		}
	}
}

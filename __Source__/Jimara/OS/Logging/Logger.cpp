#include "Logger.h"
#include <iostream>
#ifdef _WIN32
#include <windows.h>
//#include "StackWalker.h"
#else
#endif


namespace Jimara {
	namespace OS {
		void Logger::BasicLog(LogLevel level, const char* message, int exitCode, size_t stackOffset) {
#ifdef NDEBUG
			if (level == LogLevel::LOG_DEBUG) return;
#endif
			if (level < m_minLogLevel) return;
			LogInfo info = {};
			info.level = level;
			info.message = message;
			info.exitCode = exitCode;
			std::unique_lock<std::recursive_mutex> lock(m_logLock);
#ifdef _WIN32
			// __TODO__: Get win32 stack trace
#else
			// __TODO__: Get glibc stack trace
#endif
			Log(info);

			if (level == LogLevel::LOG_FATAL) {
				std::cerr << message << std::endl;
				exit(exitCode);
			}
		}
	}
}

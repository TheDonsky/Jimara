#include "StreamLogger.h"
#include <termcolor/termcolor.hpp>

namespace Jimara {
	namespace OS {
		namespace {
			typedef std::ostream& (*TerminalColor)(std::ostream&);

			static TerminalColor Color(Logger::LogLevel level) {
				static const uint8_t COUNT = static_cast<uint8_t>(Logger::LogLevel::LOG_LEVEL_COUNT);
				static const TerminalColor* COLORS = []() {
					static TerminalColor colors[COUNT];
					for (int i = 0; i < COUNT; i++) colors[i] = termcolor::reset;
					colors[static_cast<uint8_t>(Logger::LogLevel::LOG_DEBUG)] = termcolor::green;
					colors[static_cast<uint8_t>(Logger::LogLevel::LOG_INFO)] = termcolor::cyan;
					colors[static_cast<uint8_t>(Logger::LogLevel::LOG_WARNING)] = termcolor::yellow;
					colors[static_cast<uint8_t>(Logger::LogLevel::LOG_ERROR)] = termcolor::red;
					colors[static_cast<uint8_t>(Logger::LogLevel::LOG_FATAL)] = termcolor::magenta;
					return colors;
				}();
				uint8_t index = static_cast<uint8_t>(level);
				return index < COUNT ? COLORS[index] : termcolor::reset;
			}

			static const char* LevelPrefix(Logger::LogLevel level) {
				static const char DEFAULT[] = "[LOG] ";
				static const uint8_t COUNT = static_cast<uint8_t>(Logger::LogLevel::LOG_LEVEL_COUNT);
				static const char** LOG_LEVEL_PREFIXES = []() {
					static const char* prefixes[COUNT];
					for (int i = 0; i < COUNT; i++) prefixes[i] = DEFAULT;
					prefixes[static_cast<uint8_t>(Logger::LogLevel::LOG_DEBUG)] = "[DEBUG] ";
					prefixes[static_cast<uint8_t>(Logger::LogLevel::LOG_INFO)] = "[INFO] ";
					prefixes[static_cast<uint8_t>(Logger::LogLevel::LOG_WARNING)] = "[WARNING] ";
					prefixes[static_cast<uint8_t>(Logger::LogLevel::LOG_ERROR)] = "[ERROR] ";
					prefixes[static_cast<uint8_t>(Logger::LogLevel::LOG_FATAL)] = "[FATAL] ";
					return prefixes;
				}();
				uint8_t index = static_cast<uint8_t>(level);
				return index < COUNT ? LOG_LEVEL_PREFIXES[index] : DEFAULT;
			}
		}

		StreamLogger::StreamLogger(LogLevel minLogLevel, std::ostream& stream, bool useColors)
			: Logger(minLogLevel), m_stream(&stream), m_useColors(useColors) {}

		void StreamLogger::Log(const LogInfo& info) {
			if (m_useColors)
				(*m_stream) << Color(info.level);
			(*m_stream) << LevelPrefix(info.level) << info.message;
			if (m_useColors)
				(*m_stream) << termcolor::reset;
			(*m_stream) << std::endl;
		}
	}
}

#pragma once
#include "../../Core/Object.h"
#include <sstream>
#include <string>
#include <mutex>

namespace Jimara {
	namespace OS {
		/// <summary>
		/// Generic logger interface for logging messages, errors and what not
		/// </summary>
		class Logger : public virtual Object {
		public:
			enum class LogLevel : uint8_t {
				/// <summary> Logs like info, but only in debug mode </summary>
				LOG_DEBUG = 0,

				/// <summary> Informational log for notifying about progress and what not </summary>
				LOG_INFO = 1,

				/// <summary> Notification, that something might not be quite right, but the program will still work fine </summary>
				LOG_WARNING = 2,

				/// <summary> Some error occured, but the application will not crash </summary>
				LOG_ERROR = 3,

				/// <summary> Fatal error; application will exit </summary>
				LOG_FATAL = 4,

				/// <summary> Not an actual log level... This just tells how many log levels are available </summary>
				LOG_LEVEL_COUNT = 5
			};

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="minLogLevel"> Minimal log level that should not be ignored </param>
			inline Logger(LogLevel minLogLevel = LogLevel::LOG_DEBUG) : m_minLogLevel(minLogLevel) { }

			/// <summary> Virtual destructor </summary>
			inline virtual ~Logger() {}

			/// <summary> Minimal log level that should not be ignored </summary>
			inline LogLevel MinLogLevel()const { return m_minLogLevel; }

			/// <summary> Minimal log level that should not be ignored </summary>
			inline LogLevel& MinLogLevel() { return m_minLogLevel; }

			/// <summary>
			/// Generic log call
			/// </summary>
			/// <typeparam name="MessageTypes"> Message object types (const char* a string, or something, that can be output via a regular std::ostream) </typeparam>
			/// <param name="level"> Log level </param>
			/// <param name="...message"> Log message objects </param>
			template<typename... MessageTypes>
			inline void Log(LogLevel level, const MessageTypes&... message) { BasicLog(level, 0, message...); }


			/// <summary>
			/// equivalent of Log(LogLevel::LOG_DEBUG, message)
			/// </summary>
			/// <typeparam name="MessageTypes"> Message object types (const char* a string, or something, that can be output via a regular std::ostream) </typeparam>
			/// <param name="...message"> Log message objects </param>
			template<typename... MessageTypes>
			inline void Debug(const MessageTypes&... message) {
#ifndef NDEBUG
				BasicLog(LogLevel::LOG_DEBUG, 0, message...);
#endif
			}

			/// <summary>
			/// equivalent of Log(LogLevel::LOG_INFO, message)
			/// </summary>
			/// <typeparam name="MessageTypes"> Message object types (const char* a string, or something, that can be output via a regular std::ostream) </typeparam>
			/// <param name="...message"> Log message objects </param>
			template<typename... MessageTypes>
			inline void Info(const MessageTypes&... message) { BasicLog(LogLevel::LOG_INFO, 0, message...); }

			/// <summary>
			/// equivalent of Log(LogLevel::LOG_WARNING, message)
			/// </summary>
			/// <typeparam name="MessageTypes"> Message object types (const char* a string, or something, that can be output via a regular std::ostream) </typeparam>
			/// <param name="...message"> Log message objects </param>
			template<typename... MessageTypes>
			inline void Warning(const MessageTypes&... message) { BasicLog(LogLevel::LOG_WARNING, 0, message...); }

			/// <summary>
			/// equivalent of Log(LogLevel::LOG_ERROR, message)
			/// </summary>
			/// <typeparam name="MessageTypes"> Message object types (const char* a string, or something, that can be output via a regular std::ostream) </typeparam>
			/// <param name="...message"> Log message objects </param>
			template<typename... MessageTypes>
			inline void Error(const MessageTypes&... message) { BasicLog(LogLevel::LOG_ERROR, 0, message...); }

			/// <summary>
			/// equivalent of Log(LogLevel::LOG_FATAL, message)
			/// </summary>
			/// <typeparam name="MessageTypes"> Message object types (const char* a string, or something, that can be output via a regular std::ostream) </typeparam>
			/// <param name="...message"> Log message objects </param>
			template<typename... MessageTypes>
			inline void Fatal(const MessageTypes&... message) { BasicLog(LogLevel::LOG_FATAL, 0, message...); }



			/// <summary> Log call information </summary>
			struct LogInfo {
				/// <summary> Log level </summary>
				LogLevel level;

				/// <summary> Log message (memory managed externally) </summary>
				const char* message;
			};



		protected:
			/// <summary>
			/// Should record/display the log (no synchronisation needed; it's already thread-safe by design)
			/// </summary>
			/// <param name="info"> Log information </param>
			virtual void Log(const LogInfo& info) = 0;



		private:
			// Lock
			std::recursive_mutex m_logLock;

			// Minimal log level that will not be ignored
			LogLevel m_minLogLevel;

			// Underlying log logic
			void BasicLog(LogLevel level, size_t stackOffset, const char* message);

			// Base case for Output
			inline static void Output(std::ostream&) {}

			// Utility for concatenating message chunks
			template<typename First, typename... Rest>
			inline static void Output(std::ostream& stream, const First& first, const Rest&... rest) {
				stream << first;
				Output(stream, rest...);
			}

			// Underlying log logic (generic)
			template<typename... MessageTypes>
			inline void BasicLog(LogLevel level, size_t stackOffset, const MessageTypes&... messages) {
				std::stringstream stream;
				Output(stream, messages...);
				BasicLog(level, stackOffset + 1, stream.str().c_str());
			}

			// Underlying log logic (for std::string)
			template<>
			inline void BasicLog<std::string>(LogLevel level, size_t stackOffset, const std::string& message) {
				BasicLog(level, stackOffset + 1, message.c_str(), 1);
			}
		};
	}
}

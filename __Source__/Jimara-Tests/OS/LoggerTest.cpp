#include "../GtestHeaders.h"
#include <vector>
#include "OS/Logging/Logger.h"

namespace Jimara {
	namespace OS {
		namespace {
			class MockLogger : public virtual Logger {
			public:
				struct LogInformation {
					std::string message;
					int exitCode;
				};

			private:
				std::vector<LogInformation> infos[static_cast<uint8_t>(LogLevel::FATAL) + 1];

			public:
				inline MockLogger(LogLevel level = LogLevel::DEBUG) : Logger(level) {}

				inline const std::vector<LogInformation>& operator[](LogLevel level)const {
					return infos[static_cast<uint8_t>(level)];
				}

			protected:
				inline virtual void Log(const LogInfo& info) {
					infos[static_cast<uint8_t>(info.level)].push_back({ info.message, info.exitCode });
				}
			};
		}
		
		// Basic test for Logger::Debug()
		TEST(LoggerTest, Debug) {
			MockLogger logger;
			
			const char* FIRST_MESSAGE = "This shit is a debug message";
			logger.Debug(FIRST_MESSAGE);
#ifndef NDEBUG
			EXPECT_EQ(logger[Logger::LogLevel::DEBUG].size(), 1);
			EXPECT_EQ(logger[Logger::LogLevel::DEBUG][0].message, FIRST_MESSAGE);
#else
			EXPECT_EQ(logger[Logger::LogLevel::DEBUG].size(), 0);
#endif
			
			const std::string SECOND_MESSAGE = "This is another message";
			logger.Debug(SECOND_MESSAGE);
#ifndef NDEBUG
			EXPECT_EQ(logger[Logger::LogLevel::DEBUG].size(), 2);
			EXPECT_EQ(logger[Logger::LogLevel::DEBUG][0].message, FIRST_MESSAGE);
			EXPECT_EQ(logger[Logger::LogLevel::DEBUG][1].message, SECOND_MESSAGE);
#else
			EXPECT_EQ(logger[Logger::LogLevel::DEBUG].size(), 0);
#endif
		}

		// Tests for min log level
		TEST(LoggerTest, MinLogLevel) {
			const char* MESSAGE = "Some message";
			{
				MockLogger logger;
				
				logger.Info(MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::INFO].size(), 1);
				EXPECT_EQ(logger[Logger::LogLevel::INFO][0].message, MESSAGE);

				logger.MinLogLevel() = Logger::LogLevel::WARNING;
				logger.Info(MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::INFO].size(), 1);
				EXPECT_EQ(logger[Logger::LogLevel::INFO][0].message, MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::WARNING].size(), 0);

				logger.Warning(MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::INFO].size(), 1);
				EXPECT_EQ(logger[Logger::LogLevel::INFO][0].message, MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::WARNING].size(), 1);
				EXPECT_EQ(logger[Logger::LogLevel::WARNING][0].message, MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::ERROR].size(), 0);

				logger.Error(MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::INFO].size(), 1);
				EXPECT_EQ(logger[Logger::LogLevel::INFO][0].message, MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::WARNING].size(), 1);
				EXPECT_EQ(logger[Logger::LogLevel::WARNING][0].message, MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::ERROR].size(), 1);
				EXPECT_EQ(logger[Logger::LogLevel::ERROR][0].message, MESSAGE);

				logger.MinLogLevel() = Logger::LogLevel::DEBUG;
				logger.Info(MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::INFO].size(), 2);
				EXPECT_EQ(logger[Logger::LogLevel::INFO][1].message, MESSAGE);
			}
			{
				MockLogger logger(Logger::LogLevel::WARNING);
				logger.Info(MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::INFO].size(), 0);

				logger.Warning(MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::INFO].size(), 0);
				EXPECT_EQ(logger[Logger::LogLevel::WARNING].size(), 1);
				EXPECT_EQ(logger[Logger::LogLevel::WARNING][0].message, MESSAGE);

				logger.MinLogLevel() = Logger::LogLevel::DEBUG;
				logger.Info(MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::INFO].size(), 1);
				EXPECT_EQ(logger[Logger::LogLevel::INFO][0].message, MESSAGE);
			}
		}

		// Basic test for Logger::Fatal()
		TEST(LoggerTest, Fatal) {
			MockLogger logger;
			ASSERT_DEATH({ logger.Fatal("Yep, this is fatal"); }, "Yep, this is fatal");
		}
	}
}

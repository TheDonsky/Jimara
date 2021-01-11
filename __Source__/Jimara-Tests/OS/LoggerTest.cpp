#include "../GtestHeaders.h"
#include <vector>
#include "OS/Logging/StreamLogger.h"

namespace Jimara {
	namespace OS {
		namespace {
			class MockLogger : public virtual StreamLogger {
			public:
				struct LogInformation {
					std::string message;
					int exitCode;
				};

			private:
				std::vector<LogInformation> infos[static_cast<uint8_t>(LogLevel::LOG_LEVEL_COUNT)];

			public:
				inline MockLogger(LogLevel level = LogLevel::LOG_DEBUG) : StreamLogger(level) {}

				inline const std::vector<LogInformation>& operator[](LogLevel level)const {
					return infos[static_cast<uint8_t>(level)];
				}

			protected:
				inline virtual void Log(const LogInfo& info) override {
					StreamLogger::Log(info);
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
			EXPECT_EQ(logger[Logger::LogLevel::LOG_DEBUG].size(), 1);
			EXPECT_EQ(logger[Logger::LogLevel::LOG_DEBUG][0].message, FIRST_MESSAGE);
#else
			EXPECT_EQ(logger[Logger::LogLevel::DEBUG].size(), 0);
#endif
			
			const std::string SECOND_MESSAGE = "This is another message";
			logger.Debug(SECOND_MESSAGE);
#ifndef NDEBUG
			EXPECT_EQ(logger[Logger::LogLevel::LOG_DEBUG].size(), 2);
			EXPECT_EQ(logger[Logger::LogLevel::LOG_DEBUG][0].message, FIRST_MESSAGE);
			EXPECT_EQ(logger[Logger::LogLevel::LOG_DEBUG][1].message, SECOND_MESSAGE);
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
				EXPECT_EQ(logger[Logger::LogLevel::LOG_INFO].size(), 1);
				EXPECT_EQ(logger[Logger::LogLevel::LOG_INFO][0].message, MESSAGE);

				logger.MinLogLevel() = Logger::LogLevel::LOG_WARNING;
				logger.Info(MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::LOG_INFO].size(), 1);
				EXPECT_EQ(logger[Logger::LogLevel::LOG_INFO][0].message, MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::LOG_WARNING].size(), 0);

				logger.Warning(MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::LOG_INFO].size(), 1);
				EXPECT_EQ(logger[Logger::LogLevel::LOG_INFO][0].message, MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::LOG_WARNING].size(), 1);
				EXPECT_EQ(logger[Logger::LogLevel::LOG_WARNING][0].message, MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::LOG_ERROR].size(), 0);

				logger.Error(MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::LOG_INFO].size(), 1);
				EXPECT_EQ(logger[Logger::LogLevel::LOG_INFO][0].message, MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::LOG_WARNING].size(), 1);
				EXPECT_EQ(logger[Logger::LogLevel::LOG_WARNING][0].message, MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::LOG_ERROR].size(), 1);
				EXPECT_EQ(logger[Logger::LogLevel::LOG_ERROR][0].message, MESSAGE);

				logger.MinLogLevel() = Logger::LogLevel::LOG_DEBUG;
				logger.Info(MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::LOG_INFO].size(), 2);
				EXPECT_EQ(logger[Logger::LogLevel::LOG_INFO][1].message, MESSAGE);
			}
			{
				MockLogger logger(Logger::LogLevel::LOG_WARNING);
				logger.Info(MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::LOG_INFO].size(), 0);

				logger.Warning(MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::LOG_INFO].size(), 0);
				EXPECT_EQ(logger[Logger::LogLevel::LOG_WARNING].size(), 1);
				EXPECT_EQ(logger[Logger::LogLevel::LOG_WARNING][0].message, MESSAGE);

				logger.MinLogLevel() = Logger::LogLevel::LOG_DEBUG;
				logger.Info(MESSAGE);
				EXPECT_EQ(logger[Logger::LogLevel::LOG_INFO].size(), 1);
				EXPECT_EQ(logger[Logger::LogLevel::LOG_INFO][0].message, MESSAGE);
			}
		}

		// Basic test for Logger::Fatal()
		TEST(LoggerTest, Fatal) {
			MockLogger logger;
			ASSERT_DEATH({ logger.Fatal("Yep, this is fatal"); }, "Yep, this is fatal");
		}

		// Basic test for colorisation
		TEST(LoggerTest, Colors_VISUAL_ONLY) {
			MockLogger logger;
			std::cout << "This test can not fail; Just look at the colors and fix if there's something wrong with them..." << std::endl;
			logger.Debug("Debug log has this color");
			logger.Info("Info log has this color");
			logger.Warning("I warn you to recognize warnings with this color");
			logger.Error("Errors should be hightligted with this color");
			EXPECT_DEATH({ logger.Fatal("Fatal errors have this color"); }, "");
			std::cout << "Make sure the color is back to default on this line..." << std::endl;
		}
	}
}

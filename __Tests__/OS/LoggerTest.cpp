#include <gtest/gtest.h>
#include <vector>
#include "OS/Logging/Logger.h"

namespace Jimara {
	namespace OS {
		namespace {
			class MockLogger : public virtual Logger {
			public:
				struct Info {
					std::string message;
					int exitCode;
				};

				std::vector<Info> infos[static_cast<uint8_t>(LogLevel::FATAL) + 1];


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
			uint8_t index = static_cast<uint8_t>(Logger::LogLevel::DEBUG);
#ifndef NDEBUG
			EXPECT_EQ(logger.infos[index].size(), 1);
			EXPECT_EQ(logger.infos[index][0].message, FIRST_MESSAGE);
#else
			EXPECT_EQ(logger.infos[index].size(), 0);
#endif
			
			const std::string SECOND_MESSAGE = "This is another message";
			logger.Debug(SECOND_MESSAGE);
#ifndef NDEBUG
			EXPECT_EQ(logger.infos[index].size(), 2);
			EXPECT_EQ(logger.infos[index][0].message, FIRST_MESSAGE);
			EXPECT_EQ(logger.infos[index][1].message, SECOND_MESSAGE);
#else
			EXPECT_EQ(logger.infos[index].size(), 0);
#endif
		}

		// Basic test for Logger::Fatal()
		TEST(LoggerTest, Fatal) {
			MockLogger logger;
			ASSERT_DEATH({ logger.Fatal("Yep, this is fatal"); }, "Yep, this is fatal");
		}
	}
}

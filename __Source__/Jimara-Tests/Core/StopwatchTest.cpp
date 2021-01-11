#include "../GtestHeaders.h"
#include "Core/Stopwatch.h"
#include <thread>


namespace Jimara {
	namespace {
		// Let's be generous with errors... DEBUG mode and some PC-s might just be slow enough for false negatives
		static const float MAX_ERROR = 0.1f;
	}

	// Simple timer test test
	TEST(StopwatchTest, BasicTimer) {
		Stopwatch stopwatch;
		EXPECT_TRUE(stopwatch.Elapsed() >= 0.0f);
		EXPECT_TRUE(stopwatch.Elapsed() < MAX_ERROR);
		std::this_thread::sleep_for(std::chrono::seconds(1));
		EXPECT_TRUE(stopwatch.Elapsed() >= 1.0f);
		EXPECT_TRUE(stopwatch.Elapsed() < (1.0f + MAX_ERROR));
	}

	// Stop/Resume functionality
	TEST(StopwatchTest, BasicStop) {
		Stopwatch stopwatch;
		Stopwatch other;
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
			float elapsed = stopwatch.Stop();
			EXPECT_TRUE(elapsed >= 1.0f);
			EXPECT_TRUE(elapsed < (1.0f + MAX_ERROR));
		}
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
			EXPECT_TRUE(other.Elapsed() >= 2.0f);
			EXPECT_TRUE(other.Elapsed() < (2.0f + MAX_ERROR));

			float elapsed = stopwatch.Elapsed();
			EXPECT_TRUE(elapsed >= 1.0f);
			EXPECT_TRUE(elapsed < (1.0f + MAX_ERROR));
		}
		{
			stopwatch.Stop();
			std::this_thread::sleep_for(std::chrono::seconds(1));
			EXPECT_TRUE(other.Elapsed() >= 3.0f);
			EXPECT_TRUE(other.Elapsed() < (3.0f + MAX_ERROR));

			float elapsed = stopwatch.Elapsed();
			EXPECT_TRUE(elapsed >= 1.0f);
			EXPECT_TRUE(elapsed < (1.0f + MAX_ERROR));

			std::this_thread::sleep_for(std::chrono::seconds(1));
			EXPECT_EQ(elapsed, stopwatch.Elapsed());
		}
		{
			stopwatch.Resume();
			std::this_thread::sleep_for(std::chrono::seconds(1));
			EXPECT_TRUE(other.Elapsed() >= 5.0f);
			EXPECT_TRUE(other.Elapsed() < (5.0f + MAX_ERROR));

			float elapsed = stopwatch.Elapsed();
			EXPECT_TRUE(elapsed >= 2.0f);
			EXPECT_TRUE(elapsed < (2.0f + MAX_ERROR));
		}
	}

	// Reset function
	TEST(StopwatchTest, BasicReset) {
		Stopwatch stopwatch;
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
			EXPECT_TRUE(stopwatch.Elapsed() >= 1.0f);
			EXPECT_TRUE(stopwatch.Elapsed() < (1.0f + MAX_ERROR));
		}
		{
			stopwatch.Reset();
			std::this_thread::sleep_for(std::chrono::seconds(1));
			EXPECT_TRUE(stopwatch.Elapsed() >= 1.0f);
			EXPECT_TRUE(stopwatch.Elapsed() < (1.0f + MAX_ERROR));
		}
		{
			stopwatch.Stop();
			std::this_thread::sleep_for(std::chrono::seconds(1));
			EXPECT_TRUE(stopwatch.Elapsed() >= 1.0f);
			EXPECT_TRUE(stopwatch.Elapsed() < (1.0f + MAX_ERROR));
		}
		{
			stopwatch.Reset();
			std::this_thread::sleep_for(std::chrono::seconds(1));
			EXPECT_TRUE(stopwatch.Elapsed() >= 0.0f);
			EXPECT_TRUE(stopwatch.Elapsed() < MAX_ERROR);
		}
		{
			stopwatch.Resume();
			std::this_thread::sleep_for(std::chrono::seconds(1));
			EXPECT_TRUE(stopwatch.Elapsed() >= 1.0f);
			EXPECT_TRUE(stopwatch.Elapsed() < (1.0f + MAX_ERROR));
		}
	}
}

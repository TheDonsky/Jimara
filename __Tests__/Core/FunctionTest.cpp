#include <gtest/gtest.h>
#include "Core/Function.h"

namespace Jimara {
	namespace {
		// Count of total calls
		static volatile std::atomic<std::size_t> g_totalCallCount = 0;
		
		// Count of non-member function calls
		static volatile std::atomic<std::size_t> g_staticFunctionCallCount = 0;

		// Count of static member function calls
		static volatile std::atomic<std::size_t> g_staticMethodCallCount = 0;

		// Count of static lambda function calls
		static volatile std::atomic<std::size_t> g_staticLambdaCallCount = 0;

		// Resets call counters
		inline static void ResetCounts() {
			g_totalCallCount = g_staticFunctionCallCount = g_staticMethodCallCount = g_staticLambdaCallCount = 0;
		}

		// Some class with much needed members
		class SomeClass {
		public:
			volatile std::atomic<std::size_t> m_memberMethodCallCount;

			SomeClass() : m_memberMethodCallCount(0) {}

			inline void MemberCallback() {
				g_totalCallCount++;
				m_memberMethodCallCount++;
			}

			inline size_t MemberMethod() {
				MemberCallback();
				return m_memberMethodCallCount;
			}

			inline static void StaticCallback() {
				g_totalCallCount++;
				g_staticMethodCallCount++;
			}

			inline static size_t StaticMethod() {
				StaticCallback();
				return g_staticMethodCallCount;
			}
		};

		inline static void StaticCallback() {
			g_totalCallCount++;
			g_staticFunctionCallCount++;
		}

		inline static size_t StaticFunction() {
			StaticCallback();
			return g_staticFunctionCallCount;
		}
	}

	// Tests for non-member functions
	TEST(FunctionTest, StaticFunctionTest) {
		ResetCounts();
		{
			Callback<> callback = StaticCallback;
			EXPECT_EQ(g_totalCallCount, 0);
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(g_staticMethodCallCount, 0);
			EXPECT_EQ(g_staticLambdaCallCount, 0);
			callback();
			EXPECT_EQ(g_totalCallCount, 1);
			EXPECT_EQ(g_staticFunctionCallCount, 1);
			EXPECT_EQ(g_staticMethodCallCount, 0);
			EXPECT_EQ(g_staticLambdaCallCount, 0);
			callback();
			EXPECT_EQ(g_totalCallCount, 2);
			EXPECT_EQ(g_staticFunctionCallCount, 2);
			EXPECT_EQ(g_staticMethodCallCount, 0);
			EXPECT_EQ(g_staticLambdaCallCount, 0);
		}
		{
			Function<size_t> function = StaticFunction;
			EXPECT_EQ(g_totalCallCount, 2);
			EXPECT_EQ(g_staticFunctionCallCount, 2);
			EXPECT_EQ(g_staticMethodCallCount, 0);
			EXPECT_EQ(g_staticLambdaCallCount, 0);
			EXPECT_EQ(function(), 3);
			EXPECT_EQ(g_totalCallCount, 3);
			EXPECT_EQ(g_staticFunctionCallCount, 3);
			EXPECT_EQ(g_staticMethodCallCount, 0);
			EXPECT_EQ(g_staticLambdaCallCount, 0);
		}
	}

	// Tests for static member functions
	TEST(FunctionTest, StaticMethodTest) {
		ResetCounts();
		{
			Callback<> callback = SomeClass::StaticCallback;
			EXPECT_EQ(g_totalCallCount, 0);
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(g_staticMethodCallCount, 0);
			EXPECT_EQ(g_staticLambdaCallCount, 0);
			callback();
			EXPECT_EQ(g_totalCallCount, 1);
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(g_staticMethodCallCount, 1);
			EXPECT_EQ(g_staticLambdaCallCount, 0);
			callback();
			EXPECT_EQ(g_totalCallCount, 2);
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(g_staticMethodCallCount, 2);
			EXPECT_EQ(g_staticLambdaCallCount, 0);
		}
		{
			Function<size_t> function = SomeClass::StaticMethod;
			EXPECT_EQ(g_totalCallCount, 2);
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(g_staticMethodCallCount, 2);
			EXPECT_EQ(g_staticLambdaCallCount, 0);
			EXPECT_EQ(function(), 3);
			EXPECT_EQ(g_totalCallCount, 3);
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(g_staticMethodCallCount, 3);
			EXPECT_EQ(g_staticLambdaCallCount, 0);
		}
	}

	// Tests for instance member functions
	TEST(FunctionTest, InstanceMethodTest) {
		ResetCounts();
		{
			SomeClass instance;
			Callback<> callback = Callback<>(&SomeClass::MemberCallback, instance);
			EXPECT_EQ(g_totalCallCount, 0);
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(g_staticMethodCallCount, 0);
			EXPECT_EQ(g_staticLambdaCallCount, 0);
			EXPECT_EQ(instance.m_memberMethodCallCount, 0);
			callback();
			EXPECT_EQ(g_totalCallCount, 1);
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(g_staticMethodCallCount, 0);
			EXPECT_EQ(g_staticLambdaCallCount, 0);
			EXPECT_EQ(instance.m_memberMethodCallCount, 1);
			callback();
			EXPECT_EQ(g_totalCallCount, 2);
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(g_staticMethodCallCount, 0);
			EXPECT_EQ(g_staticLambdaCallCount, 0);
			EXPECT_EQ(instance.m_memberMethodCallCount, 2);
		}
		{
			SomeClass instance;
			Function<size_t> function = Function<size_t>(&SomeClass::MemberMethod, instance);
			EXPECT_EQ(g_totalCallCount, 2);
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(g_staticMethodCallCount, 0);
			EXPECT_EQ(g_staticLambdaCallCount, 0);
			EXPECT_EQ(instance.m_memberMethodCallCount, 0);
			EXPECT_EQ(function(), 1);
			EXPECT_EQ(g_totalCallCount, 3);
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(g_staticMethodCallCount, 0);
			EXPECT_EQ(g_staticLambdaCallCount, 0);
			EXPECT_EQ(instance.m_memberMethodCallCount, 1);
		}
	}

	// Tests for simple lambdas (not all are possible)
	TEST(FunctionTest, StaticLambdaTest) {
		ResetCounts();
		{
			Callback<> callback = []() { g_totalCallCount++; g_staticLambdaCallCount++; };
			EXPECT_EQ(g_totalCallCount, 0);
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(g_staticMethodCallCount, 0);
			EXPECT_EQ(g_staticLambdaCallCount, 0);
			callback();
			EXPECT_EQ(g_totalCallCount, 1);
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(g_staticMethodCallCount, 0);
			EXPECT_EQ(g_staticLambdaCallCount, 1);
			callback();
			EXPECT_EQ(g_totalCallCount, 2);
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(g_staticMethodCallCount, 0);
			EXPECT_EQ(g_staticLambdaCallCount, 2);
		}
		{
			Function<size_t> function = []() { g_totalCallCount++; g_staticLambdaCallCount++; return (size_t)g_staticLambdaCallCount; };
			EXPECT_EQ(g_totalCallCount, 2);
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(g_staticMethodCallCount, 0);
			EXPECT_EQ(g_staticLambdaCallCount, 2);
			EXPECT_EQ(function(), 3);
			EXPECT_EQ(g_totalCallCount, 3);
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(g_staticMethodCallCount, 0);
			EXPECT_EQ(g_staticLambdaCallCount, 3);
		}
	}
}

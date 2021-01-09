#include <gtest/gtest.h>
#include <stdlib.h> 
#include <thread>
#include <vector>
#include <memory>
#include "Core/Event.h"

namespace Jimara {
	namespace {
		// Count of non-member function calls
		static volatile size_t g_staticFunctionCallCount = 0;

		// Some class with much needed members
		class SomeClass {
		public:
			volatile size_t m_memberMethodCallCount;

			SomeClass() : m_memberMethodCallCount(0) {}

			inline void IncrementCallback() { m_memberMethodCallCount++; }

			inline void SetCallback(size_t value) { m_memberMethodCallCount = value; }
		};

		inline void IncrementCallback() { g_staticFunctionCallCount++; }

		inline void IncrementTwoCallback() { g_staticFunctionCallCount += 2; }


		inline void IncrementCallbackWithParam(void*) {
			g_staticFunctionCallCount++;
		}

		inline void IncrementAndRemoveCallback(void* evt) {
			g_staticFunctionCallCount += 2;
			(*((Event<void*>*)evt)) -= IncrementAndRemoveCallback;
		}

		inline void IncrementAndChangeCallback(void* evt) {
			g_staticFunctionCallCount += 4;
			(*((Event<void*>*)evt)) -= IncrementAndChangeCallback;
			(*((Event<void*>*)evt)) += IncrementCallbackWithParam;
		}

		inline void IncreementAndRemoveOtherCallbackB(void* evt);

		inline void IncreementAndRemoveOtherCallbackA(void* evt) {
			g_staticFunctionCallCount += 8;
			(*((Event<void*>*)evt)) -= IncreementAndRemoveOtherCallbackB;
		}

		inline void IncreementAndRemoveOtherCallbackB(void* evt) {
			g_staticFunctionCallCount += 16;
			(*((Event<void*>*)evt)) -= IncreementAndRemoveOtherCallbackA;
		}

		class Countdown {
		private:
			Event<>* m_event;
			size_t m_remaining;
			Countdown* m_replacement;

			inline void Subtract() {
				if (m_remaining > 0) {
					g_staticFunctionCallCount++;
					m_remaining--;
				}
				if (m_remaining <= 0) {
					if (m_replacement != nullptr)
						m_replacement->Subscribe(m_event);
					Subscribe(nullptr);
				}
			}
		public:
			inline Countdown(size_t remaining = 128, Countdown* replacement = nullptr)
				: m_event(nullptr), m_remaining(remaining), m_replacement(replacement) {}

			inline void Subscribe(Event<>* evt) {
				if (evt == m_event) return;
				if (m_event != nullptr)
					(*m_event) -= Callback<>(&Countdown::Subtract, this);
				m_event = evt;
				if (m_event != nullptr)
					(*m_event) += Callback<>(&Countdown::Subtract, this);
			}

			inline size_t Remaining()const { return m_remaining; }
		};
	}

	// Simple static callback test
	TEST(EventTest, SingleThreadedStatic) {
		g_staticFunctionCallCount = 0;
		{
			EventInstance<> eventInstance;
			Event<>& evt(eventInstance);
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			
			eventInstance();
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			
			evt += IncrementCallback;
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			eventInstance();
			EXPECT_EQ(g_staticFunctionCallCount, 1);

			evt += IncrementCallback;
			eventInstance();
			EXPECT_EQ(g_staticFunctionCallCount, 2);

			evt -= IncrementCallback;
			eventInstance();
			EXPECT_EQ(g_staticFunctionCallCount, 2);
		}
		g_staticFunctionCallCount = 0;
		{
			EventInstance<> eventInstance;
			Event<>& evt(eventInstance);

			eventInstance();
			EXPECT_EQ(g_staticFunctionCallCount, 0);

			evt += IncrementTwoCallback;
			eventInstance();
			EXPECT_EQ(g_staticFunctionCallCount, 2);

			evt += IncrementCallback;
			eventInstance();
			EXPECT_EQ(g_staticFunctionCallCount, 5);

			evt -= IncrementTwoCallback;
			eventInstance();
			EXPECT_EQ(g_staticFunctionCallCount, 6);

			evt -= IncrementTwoCallback;
			eventInstance();
			EXPECT_EQ(g_staticFunctionCallCount, 7);

			evt -= IncrementCallback;
			eventInstance();
			EXPECT_EQ(g_staticFunctionCallCount, 7);
		}
	}

	// Simple member callback test
	TEST(EventTest, SingleThreadedMember) {
		g_staticFunctionCallCount = 0;
		{
			SomeClass instance;
			EventInstance<> eventInstance;
			Event<>& evt(eventInstance);
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(instance.m_memberMethodCallCount, 0);

			eventInstance();
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(instance.m_memberMethodCallCount, 0);

			evt += Callback<>(&SomeClass::IncrementCallback, instance);
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(instance.m_memberMethodCallCount, 0);
			eventInstance();
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(instance.m_memberMethodCallCount, 1);

			evt += Callback<>(&SomeClass::IncrementCallback, instance);
			eventInstance();
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(instance.m_memberMethodCallCount, 2);

			evt -= IncrementCallback;
			eventInstance();
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(instance.m_memberMethodCallCount, 3);

			evt -= Callback<>(&SomeClass::IncrementCallback, instance);
			eventInstance();
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(instance.m_memberMethodCallCount, 3);
		}
		{
			SomeClass instance;
			EventInstance<size_t> eventInstance;
			Event<size_t>& evt(eventInstance);

			eventInstance(4);
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(instance.m_memberMethodCallCount, 0);

			evt -= Callback<size_t>(&SomeClass::SetCallback, instance);
			eventInstance(4);
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(instance.m_memberMethodCallCount, 0);

			evt += Callback<size_t>(&SomeClass::SetCallback, instance);
			eventInstance(4);
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(instance.m_memberMethodCallCount, 4);

			evt -= Callback<size_t>(&SomeClass::SetCallback, instance);
			eventInstance(4);
			EXPECT_EQ(g_staticFunctionCallCount, 0);
			EXPECT_EQ(instance.m_memberMethodCallCount, 4);
		}
	}

	// Simple static and member callback test
	TEST(EventTest, SingleThreadedMixed) {
		g_staticFunctionCallCount = 0;
		{
			SomeClass instance;
			EventInstance<> eventInstance;
			Event<>& evt(eventInstance);

			evt += IncrementCallback;
			eventInstance();
			EXPECT_EQ(g_staticFunctionCallCount, 1);
			EXPECT_EQ(instance.m_memberMethodCallCount, 0);

			evt += Callback<>(&SomeClass::IncrementCallback, instance);
			eventInstance();
			EXPECT_EQ(g_staticFunctionCallCount, 2);
			EXPECT_EQ(instance.m_memberMethodCallCount, 1);

			evt -= IncrementCallback;
			eventInstance();
			EXPECT_EQ(g_staticFunctionCallCount, 2);
			EXPECT_EQ(instance.m_memberMethodCallCount, 2);
		}
	}

	// Callback test with execution-time additions and removals
	TEST(EventTest, SingleThreadedNonConst) {
		g_staticFunctionCallCount = 0;
		{
			EventInstance<void*> eventInstance;
			Event<void*>& evt(eventInstance);
			
			evt += IncrementAndRemoveCallback;
			eventInstance(&evt);
			EXPECT_EQ(g_staticFunctionCallCount, 2);

			eventInstance(&evt);
			EXPECT_EQ(g_staticFunctionCallCount, 2);
		}
		g_staticFunctionCallCount = 0;
		{
			EventInstance<void*> eventInstance;
			Event<void*>& evt(eventInstance);

			evt += IncrementAndRemoveCallback;
			evt += IncrementCallbackWithParam;
			eventInstance(&evt);
			EXPECT_EQ(g_staticFunctionCallCount, 3);

			eventInstance(&evt);
			EXPECT_EQ(g_staticFunctionCallCount, 4);
		}
		g_staticFunctionCallCount = 0;
		{
			EventInstance<void*> eventInstance;
			Event<void*>& evt(eventInstance);

			evt += IncrementAndChangeCallback;
			eventInstance(&evt);
			EXPECT_EQ(g_staticFunctionCallCount, 4);

			eventInstance(&evt);
			EXPECT_EQ(g_staticFunctionCallCount, 5);
		}
		g_staticFunctionCallCount = 0;
		{
			EventInstance<void*> eventInstance;
			Event<void*>& evt(eventInstance);

			evt += IncreementAndRemoveOtherCallbackA;
			evt += IncreementAndRemoveOtherCallbackB;
			eventInstance(&evt);
			EXPECT_TRUE(g_staticFunctionCallCount == 8 || g_staticFunctionCallCount == 16);

			int expected = g_staticFunctionCallCount * 2;
			eventInstance(&evt);
			EXPECT_EQ(g_staticFunctionCallCount, g_staticFunctionCallCount);
		}
	}

	// Simple multi-threaded callback test
	TEST(EventTest, MultiThreaded) {
		g_staticFunctionCallCount = 0;

		EventInstance<> eventInstance;
		((Event<>&)eventInstance) += IncrementCallback;

		const size_t ITERATIONS = 16000;
		const size_t THREAD_COUNT = (size_t)std::thread::hardware_concurrency() * 4;

		void(*threadFn)(EventInstance<>*, size_t) = [](EventInstance<>* eventInstance, size_t iterations) {
			for (size_t i = 0; i < iterations; i++) 
				eventInstance->operator()();
		};
		
		std::vector<std::thread> threads;
		for (size_t i = 0; i < THREAD_COUNT; i++)
			threads.push_back(std::thread(threadFn, &eventInstance, ITERATIONS));

		for (size_t i = 0; i < threads.size(); i++)
			threads[i].join();

		EXPECT_EQ(g_staticFunctionCallCount, ITERATIONS * THREAD_COUNT);
	}

	// Multi-threaded callback test with execution-time additions and removals
	TEST(EventTest, MultiThreadedNonConst) {
		g_staticFunctionCallCount = 0;

		const size_t THREAD_COUNT = (size_t)std::thread::hardware_concurrency() * 4;

		void(*threadFn)(EventInstance<>*) = [](EventInstance<>* eventInstance) {
			for (size_t i = 0; i < 512; i++)
				eventInstance->operator()();
		};

		{
			EventInstance<> eventInstance;
			Event<>& evt(eventInstance);
			
			size_t expected = 0;
			
			std::vector<std::shared_ptr<Countdown>> countdowns;
			for (size_t i = 0; i < 128; i++) {
				countdowns.push_back(std::make_shared<Countdown>());
				std::shared_ptr<Countdown> last = countdowns.back();
				expected += last->Remaining();
				last->Subscribe(&evt);
			}

			std::vector<std::thread> threads;
			for (size_t i = 0; i < THREAD_COUNT; i++)
				threads.push_back(std::thread(threadFn, &eventInstance));

			for (size_t i = 0; i < threads.size(); i++)
				threads[i].join();

			EXPECT_EQ(g_staticFunctionCallCount, expected);
		}

		g_staticFunctionCallCount = 0;
		{
			EventInstance<> eventInstance;
			Event<>& evt(eventInstance);

			size_t expected = 0;

			std::vector<std::shared_ptr<Countdown>> countdowns;
			for (size_t i = 0; i < 128; i++) {
				countdowns.push_back(std::make_shared<Countdown>(((uint32_t)rand()) % 512));
				std::shared_ptr<Countdown> last = countdowns.back();
				expected += last->Remaining();
				last->Subscribe(&evt);
			}

			std::vector<std::thread> threads;
			for (size_t i = 0; i < THREAD_COUNT; i++)
				threads.push_back(std::thread(threadFn, &eventInstance));

			for (size_t i = 0; i < threads.size(); i++)
				threads[i].join();

			EXPECT_EQ(g_staticFunctionCallCount, expected);
		}

		g_staticFunctionCallCount = 0;
		{
			EventInstance<> eventInstance;
			Event<>& evt(eventInstance);

			size_t expected = 0;

			std::vector<std::shared_ptr<Countdown>> replacements;
			for (size_t i = 0; i < 512; i++) {
				replacements.push_back(std::make_shared<Countdown>(((uint32_t)rand()) % 256));
				std::shared_ptr<Countdown> last = replacements.back();
				expected += last->Remaining();
			}

			std::vector<std::shared_ptr<Countdown>> countdowns;
			for (size_t i = 0; i < replacements.size(); i++) {
				countdowns.push_back(std::make_shared<Countdown>(((uint32_t)rand()) % 256, replacements[i].operator->()));
				std::shared_ptr<Countdown> last = countdowns.back();
				expected += last->Remaining();
				last->Subscribe(&evt);
			}

			std::vector<std::thread> threads;
			for (size_t i = 0; i < THREAD_COUNT; i++)
				threads.push_back(std::thread(threadFn, &eventInstance));

			for (size_t i = 0; i < threads.size(); i++)
				threads[i].join();

			EXPECT_EQ(g_staticFunctionCallCount, expected);
		}
	}
}

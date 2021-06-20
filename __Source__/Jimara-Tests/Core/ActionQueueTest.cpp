#include "../GtestHeaders.h"
#include "../Memory.h"
#include "Core/Collections/ThreadPool.h"



namespace Jimara {
	namespace {
		class ValueSetter : public virtual Object {
		private:
			const Function<int> m_getValue;
			int* m_dst;

		public:
			inline ValueSetter(const Function<int>& getValue, int* destination) : m_getValue(getValue), m_dst(destination) {}

			inline void Set() { (*m_dst) = m_getValue(); }
		};

		static int a = 0, b = 0, c = 0;
		static std::atomic<int> counter = 0;
		static const Function<int> getValue([]() { return counter.fetch_add(1) + 1; });
		inline static void ResetValues() {
			a = b = c = 0;
			counter = 0;
		}

		struct PoolHolder : public virtual Object { 
			ThreadPool* pool; 

			inline PoolHolder(ThreadPool* p) : pool(p) {}
		};
	}


	TEST(ActionQueueTest, SynchronousActionQueue_Basic) {
		ResetValues();
		Jimara::Test::Memory::MemorySnapshot snapshot;
		{
			SynchronousActionQueue<> queue;
			queue.Schedule(Callback<Object*>([](Object*) { a = getValue(); }), nullptr);
			EXPECT_EQ(a, 0);
			EXPECT_EQ(b, 0);
			EXPECT_EQ(c, 0);
		}
		EXPECT_EQ(a, 0);
		EXPECT_EQ(b, 0);
		EXPECT_EQ(c, 0);
		{
			SynchronousActionQueue<> queue;
			queue.Schedule(Callback<Object*>([](Object*) { a = getValue(); }), nullptr);
			EXPECT_EQ(a, 0);
			EXPECT_EQ(b, 0);
			EXPECT_EQ(c, 0);
			queue.Flush();
			EXPECT_EQ(a, 1);
			EXPECT_EQ(b, 0);
			EXPECT_EQ(c, 0);
		}
		EXPECT_EQ(a, 1);
		EXPECT_EQ(b, 0);
		EXPECT_EQ(c, 0);
		{
			SynchronousActionQueue<> queue;
			queue.Schedule(Callback<Object*>([](Object*) { a = getValue(); }), nullptr);
			queue.Schedule(Callback<Object*>([](Object*) { b = getValue(); }), nullptr);
			EXPECT_EQ(a, 1);
			EXPECT_EQ(b, 0);
			EXPECT_EQ(c, 0);
			queue();
			EXPECT_EQ(a, 2);
			EXPECT_EQ(b, 3);
			EXPECT_EQ(c, 0);
		}
		EXPECT_EQ(a, 2);
		EXPECT_EQ(b, 3);
		EXPECT_EQ(c, 0);
		{
			SynchronousActionQueue<> queue;
			queue.Schedule(Callback<Object*>([](Object*) { a = getValue(); }), nullptr);
			queue.Schedule(Callback<Object*>([](Object* setter) { dynamic_cast<ValueSetter*>(setter)->Set(); }), Object::Instantiate<ValueSetter>(getValue, &c));
			queue.Schedule(Callback<Object*>([](Object*) { b = getValue(); }), nullptr);
			EXPECT_EQ(a, 2);
			EXPECT_EQ(b, 3);
			EXPECT_EQ(c, 0);
			queue();
			queue.Flush();
			queue();
			EXPECT_EQ(a, 4);
			EXPECT_EQ(b, 6);
			EXPECT_EQ(c, 5);
			queue.Schedule(Callback<Object*>([](Object* setter) { dynamic_cast<ValueSetter*>(setter)->Set(); }), Object::Instantiate<ValueSetter>(getValue, &c));
			EXPECT_EQ(a, 4);
			EXPECT_EQ(b, 6);
			EXPECT_EQ(c, 5);
			queue.Flush();
			EXPECT_EQ(a, 4);
			EXPECT_EQ(b, 6);
			EXPECT_EQ(c, 7);
		}
		EXPECT_EQ(a, 4);
		EXPECT_EQ(b, 6);
		EXPECT_EQ(c, 7);
		EXPECT_TRUE(snapshot.Compare());
	}

	TEST(ActionQueueTest, SynchronousActionQueue_ScheduleFromAction) {
		ResetValues();
		Jimara::Test::Memory::MemorySnapshot snapshot;
		{
			SynchronousActionQueue<> queue;
			static SynchronousActionQueue<>* q = nullptr;
			q = &queue;
			queue.Schedule(Callback<Object*>([](Object*) {
				q->Schedule(Callback<Object*>([](Object*) { a = getValue(); }), nullptr);
				}), nullptr);
			queue.Schedule(Callback<Object*>([](Object*) { b = getValue(); }), nullptr);
			queue.Schedule(Callback<Object*>([](Object* object) {
				q->Schedule(Callback<Object*>([](Object* setter) { dynamic_cast<ValueSetter*>(setter)->Set(); }), object);
				}), Object::Instantiate<ValueSetter>(getValue, &c));
			EXPECT_EQ(a, 0);
			queue.Flush();
			EXPECT_EQ(a, 0);
			EXPECT_EQ(b, 1);
			EXPECT_EQ(c, 0);
			queue.Flush();
			EXPECT_EQ(a, 2);
			EXPECT_EQ(b, 1);
			EXPECT_EQ(c, 3);
			queue.Flush();
			EXPECT_EQ(a, 2);
			EXPECT_EQ(b, 1);
			EXPECT_EQ(c, 3);
		}
		EXPECT_EQ(a, 2);
		EXPECT_EQ(b, 1);
		EXPECT_EQ(c, 3);
		EXPECT_TRUE(snapshot.Compare());
	}

	TEST(ActionQueueTest, ThreadPool_Basic) {
		ResetValues();
		Jimara::Test::Memory::MemorySnapshot snapshot;
		{
			EXPECT_EQ(a, 0);
			EXPECT_EQ(b, 0);
			EXPECT_EQ(c, 0);
			ThreadPool pool;
			pool.Schedule(Callback<Object*>([](Object*) { a = getValue(); }), nullptr);
			pool.Schedule(Callback<Object*>([](Object* setter) { dynamic_cast<ValueSetter*>(setter)->Set(); }), Object::Instantiate<ValueSetter>(getValue, &c));
			pool.Schedule(Callback<Object*>([](Object*) { b = getValue(); }), nullptr);
		}
		EXPECT_GT(a, 0);
		EXPECT_GT(b, 0);
		EXPECT_GT(c, 0);
		EXPECT_EQ(counter, 3);
		EXPECT_TRUE(snapshot.Compare());
	}

	TEST(ActionQueueTest, ThreadPool_ScheduleFromAction) {
		ResetValues();
		Jimara::Test::Memory::MemorySnapshot snapshot;
		{
			ThreadPool pool;
			Reference<PoolHolder> holder = Object::Instantiate<PoolHolder>(&pool);
			pool.Schedule(Callback<Object*>([](Object* h) {
				dynamic_cast<PoolHolder*>(h)->pool->Schedule(Callback<Object*>([](Object*) { a = getValue(); }), nullptr);
				}), holder);
			pool.Schedule(Callback<Object*>([](Object*) { b = getValue(); }), nullptr);
			pool.Schedule(Callback<Object*>([](Object* h) {
				dynamic_cast<PoolHolder*>(h)->pool->Schedule(
					Callback<Object*>([](Object* setter) { dynamic_cast<ValueSetter*>(setter)->Set(); }), Object::Instantiate<ValueSetter>(getValue, &c));
				}), holder);
		}
		EXPECT_GT(a, 0);
		EXPECT_GT(b, 0);
		EXPECT_GT(c, 0);
		EXPECT_EQ(counter, 3);
		EXPECT_TRUE(snapshot.Compare());
	}
}

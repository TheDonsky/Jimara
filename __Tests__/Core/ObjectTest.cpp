#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "Core/Object.h"

namespace Jimara {
	namespace {
		static volatile std::atomic<std::size_t> g_instanceCount = 0;

		// Some class that does global instance counting
		class InstanceCounter : public virtual Object {
		public:
			InstanceCounter() { g_instanceCount++; }

			virtual ~InstanceCounter() { g_instanceCount--; }
		};

		// Representative of some class derived from SomeDerivedClass
		class SomeDerivedClass : public virtual InstanceCounter {};
	}

	// Tests for on-stack memory behaviour
	TEST(ObjectTest, Stack) {
		EXPECT_EQ(g_instanceCount, 0);
		{
			InstanceCounter instance;
			EXPECT_EQ(g_instanceCount, 1);
		}
		EXPECT_EQ(g_instanceCount, 0);
		{
			InstanceCounter instance[2];
			EXPECT_EQ(g_instanceCount, 2);
		}
		EXPECT_EQ(g_instanceCount, 0);
		{
			InstanceCounter instance;
			{
				Reference<InstanceCounter> ref(&instance);
				Reference<Object> objRef = ref;
				Reference<SomeDerivedClass> derClassRef = objRef;
				EXPECT_EQ(g_instanceCount, 1);
			}
			EXPECT_EQ(g_instanceCount, 1);
		}
		EXPECT_EQ(g_instanceCount, 0);
	}

	// Test for heap manual allocation/deallocation (new & delete)
	TEST(ObjectTest, HeapManual) {
		EXPECT_EQ(g_instanceCount, 0);
		{
			InstanceCounter* counter = new InstanceCounter();
			EXPECT_EQ(g_instanceCount, 1);
			delete counter;
		}
		EXPECT_EQ(g_instanceCount, 0);
		{
			InstanceCounter* counter = new InstanceCounter[2];
			EXPECT_EQ(g_instanceCount, 2);
			delete[] counter;
		}
		EXPECT_EQ(g_instanceCount, 0);
		{
			Object* counter = new InstanceCounter();
			EXPECT_EQ(g_instanceCount, 1);
			delete counter;
		}
		EXPECT_EQ(g_instanceCount, 0);
	}

	// Tests for heap reference-counted allocation/deallocation (AddRef/ReleaseRef)
	TEST(ObjectTest, HeapManualRef) {
		EXPECT_EQ(g_instanceCount, 0);
		{
			InstanceCounter* counter = new InstanceCounter();
			EXPECT_EQ(g_instanceCount, 1);
			counter->ReleaseRef();
		}
		EXPECT_EQ(g_instanceCount, 0);
		{
			Object* counter = new InstanceCounter();
			EXPECT_EQ(g_instanceCount, 1);
			counter->ReleaseRef();
		}
		EXPECT_EQ(g_instanceCount, 0); 
		{
			Object* counter = new InstanceCounter();
			EXPECT_EQ(g_instanceCount, 1);
			counter->AddRef();
			EXPECT_EQ(g_instanceCount, 1);
			counter->ReleaseRef();
			EXPECT_EQ(g_instanceCount, 1);
			counter->ReleaseRef();
		}
		EXPECT_EQ(g_instanceCount, 0);
	}

	// Tests for allocation/deallocation when suing references (Referense<>)
	TEST(ObjectTest, HeapTestReference) {
		EXPECT_EQ(g_instanceCount, 0);
		{
			InstanceCounter* counter = new InstanceCounter();
			EXPECT_EQ(g_instanceCount, 1);
			Reference<InstanceCounter> ref(counter);
			counter->ReleaseRef();
		}
		EXPECT_EQ(g_instanceCount, 0);
		{
			Reference<InstanceCounter> ref(new InstanceCounter());
			EXPECT_EQ(g_instanceCount, 1);
			ref->ReleaseRef();
		}
		EXPECT_EQ(g_instanceCount, 0);
		{
			Reference<InstanceCounter> ref(Object::Instantiate<InstanceCounter>());
			EXPECT_EQ(g_instanceCount, 1);
		}
		EXPECT_EQ(g_instanceCount, 0);
		{
			Reference<InstanceCounter> ref = Object::Instantiate<InstanceCounter>();
			Reference<InstanceCounter> anotherRef = ref;
			Reference<Object> objRef = ref;
			Reference<SomeDerivedClass> derivedRef = ref;
			EXPECT_EQ(g_instanceCount, 1);
		}
		EXPECT_EQ(g_instanceCount, 0);
		{
			Reference<SomeDerivedClass> ref = Object::Instantiate<InstanceCounter>();
			EXPECT_EQ(g_instanceCount, 0);
			EXPECT_EQ(ref, nullptr);
		}
		EXPECT_EQ(g_instanceCount, 0);
	}


	namespace {
		// Thread for heap test
		void HeapTestThread(Reference<InstanceCounter> reference, uint32_t threadDepthLeft) {
			if (reference == nullptr || threadDepthLeft <= 0) return;
			std::vector<std::thread> threads;
			threadDepthLeft--;
			{
				threads.push_back(std::thread(HeapTestThread, reference, threadDepthLeft));
				threads.push_back(std::thread(HeapTestThread, Object::Instantiate<Object>(), threadDepthLeft));
				threads.push_back(std::thread(HeapTestThread, Object::Instantiate<InstanceCounter>(), threadDepthLeft));
				threads.push_back(std::thread(HeapTestThread, Object::Instantiate<SomeDerivedClass>(), threadDepthLeft));
			}
			for (size_t i = 0; i < threads.size(); i++)
				threads[i].join();
		}
	}
	
	// Basic multithreaded heap test
	TEST(ObjectTest, HeapTestMultithread) {
		EXPECT_EQ(g_instanceCount, 0);
		{
			Reference<SomeDerivedClass> ref = Object::Instantiate<SomeDerivedClass>();
			std::thread thread(HeapTestThread, ref, 6);
			thread.join();
			EXPECT_EQ(g_instanceCount, 1);
		}
		EXPECT_EQ(g_instanceCount, 0);
	}
}

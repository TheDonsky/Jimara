#include "gtest/gtest.h"
#include "Core/Reference.h"


namespace Jimara {
	namespace {
		class MockObject {
		private:
			mutable std::atomic<std::size_t> m_referenceCount;

		public:
			inline MockObject() : m_referenceCount(0) {}

			inline virtual ~MockObject() {}

			inline void AddRef() { m_referenceCount++; }

			inline void ReleaseRef() { m_referenceCount.fetch_sub(1); }

			inline size_t ReferenceCount() { return m_referenceCount; }
		};

		class OtherMockObject {
		private:
			mutable std::atomic<std::size_t> m_referenceCount;

		public:
			inline OtherMockObject() : m_referenceCount(0) {}
			
			inline virtual ~OtherMockObject() {}

			inline void AddRef() { m_referenceCount++; }

			inline void ReleaseRef() { m_referenceCount.fetch_sub(1); }

			inline size_t ReferenceCount() { return m_referenceCount; }
		};

		class DerivedMockObject : public virtual MockObject {};
	}

	// Basic test for reference counts
	TEST(ReferenceTest, RefCount) {
		MockObject obj[2];
		{
			Reference<MockObject> ref(obj);
			EXPECT_EQ(obj->ReferenceCount(), 1);
		}
		EXPECT_EQ(obj->ReferenceCount(), 0);
		{
			Reference<MockObject> ref;
			ref = obj;
			EXPECT_EQ(obj->ReferenceCount(), 1);
			ref = nullptr;
			EXPECT_EQ(obj->ReferenceCount(), 0);
			ref = (obj + 1);
			EXPECT_EQ(obj->ReferenceCount(), 0);
			EXPECT_EQ(obj[1].ReferenceCount(), 1);
		}
		EXPECT_EQ(obj[1].ReferenceCount(), 0);
		{
			Reference<MockObject> refA(obj);
			{
				Reference<MockObject> refB(obj);
				EXPECT_EQ(obj->ReferenceCount(), 2);
			}
			EXPECT_EQ(obj->ReferenceCount(), 1);
			{
				Reference<MockObject> refB;
				EXPECT_EQ(obj->ReferenceCount(), 1);
				refB = refA;
				EXPECT_EQ(obj->ReferenceCount(), 2);
				refB = obj;
				EXPECT_EQ(obj->ReferenceCount(), 2);
				refA = obj + 1;
				EXPECT_EQ(obj->ReferenceCount(), 1);
				EXPECT_EQ(obj[1].ReferenceCount(), 1);
			}
		}
		{
			Reference<MockObject>(*getRef)(MockObject * obj) = [](MockObject* obj) {
				Reference<MockObject> ref(obj);
				EXPECT_EQ(ref->ReferenceCount(), ref->ReferenceCount());
				return ref;
			};
			EXPECT_TRUE(getRef(obj) != nullptr);
			EXPECT_TRUE(getRef(obj) == obj);
			EXPECT_EQ(obj->ReferenceCount(), 0);
			EXPECT_EQ(getRef(obj)->ReferenceCount(), 1);
			EXPECT_EQ(obj->ReferenceCount(), 0);
			{
				Reference<MockObject> ref(getRef(obj));
				EXPECT_EQ(obj->ReferenceCount(), 1);
				EXPECT_EQ(getRef(obj)->ReferenceCount(), 2);
			}
			{
				Reference<MockObject> ref;
				EXPECT_TRUE(ref == nullptr);
				EXPECT_EQ(obj->ReferenceCount(), 0);
				ref = getRef(obj);
				EXPECT_EQ(obj->ReferenceCount(), 1);
			}
		}
	}

	// All possible comparizon checks
	TEST(ReferenceTest, Compare) {
		MockObject obj[2];
		{
			EXPECT_TRUE(Reference<MockObject>(obj) < (obj + 1));
			EXPECT_TRUE(Reference<MockObject>(obj) <= (obj));
			EXPECT_TRUE(Reference<MockObject>(obj) <= (obj + 1));
			EXPECT_TRUE(Reference<MockObject>(obj) == (obj));
			EXPECT_TRUE(Reference<MockObject>(obj) != (obj + 1));
			EXPECT_TRUE(Reference<MockObject>(obj + 1) != (obj));
			EXPECT_TRUE(Reference<MockObject>(obj) >= (obj));
			EXPECT_TRUE(Reference<MockObject>(obj + 1) >= (obj));
			EXPECT_TRUE(Reference<MockObject>(obj + 1) > (obj));
		}
		{
			EXPECT_TRUE((obj) < Reference<MockObject>(obj + 1));
			EXPECT_TRUE((obj) <= Reference<MockObject>(obj));
			EXPECT_TRUE((obj) <= Reference<MockObject>(obj + 1));
			EXPECT_TRUE((obj) == Reference<MockObject>(obj));
			EXPECT_TRUE((obj) != Reference<MockObject>(obj + 1));
			EXPECT_TRUE((obj + 1) != Reference<MockObject>(obj));
			EXPECT_TRUE((obj) >= Reference<MockObject>(obj));
			EXPECT_TRUE((obj + 1) >= Reference<MockObject>(obj));
			EXPECT_TRUE((obj + 1) > Reference<MockObject>(obj));
		}
		{
			EXPECT_TRUE(Reference<MockObject>(obj) < Reference<MockObject>(obj + 1));
			EXPECT_TRUE(Reference<MockObject>(obj) <= Reference<MockObject>(obj));
			EXPECT_TRUE(Reference<MockObject>(obj) <= Reference<MockObject>(obj + 1));
			EXPECT_TRUE(Reference<MockObject>(obj) == Reference<MockObject>(obj));
			EXPECT_TRUE(Reference<MockObject>(obj) != Reference<MockObject>(obj + 1));
			EXPECT_TRUE(Reference<MockObject>(obj + 1) != Reference<MockObject>(obj));
			EXPECT_TRUE(Reference<MockObject>(obj) >= Reference<MockObject>(obj));
			EXPECT_TRUE(Reference<MockObject>(obj + 1) >= Reference<MockObject>(obj));
			EXPECT_TRUE(Reference<MockObject>(obj + 1) > Reference<MockObject>(obj));
		}
		{
			EXPECT_TRUE(Reference<MockObject>(obj) != nullptr);
			EXPECT_TRUE(Reference<MockObject>(nullptr) == nullptr);
		}
	}

	// Basic tests for mixed types
	TEST(ReferenceTest, Casting) {
		MockObject mockObject[1];
		OtherMockObject otherObject[1];
		DerivedMockObject derivedObject[1];
		{
			EXPECT_TRUE(Reference<MockObject>(derivedObject) != nullptr);
			EXPECT_TRUE(Reference<DerivedMockObject>(derivedObject) != nullptr);
			EXPECT_TRUE(Reference<DerivedMockObject>(Reference<MockObject>(mockObject)) == nullptr);
			EXPECT_TRUE(Reference<DerivedMockObject>(Reference<MockObject>(derivedObject)) != nullptr);
		}
		{
			Reference<MockObject> refObject(mockObject);
			Reference<MockObject> refDerivedObject(derivedObject);
			Reference<OtherMockObject> refOther(otherObject);
			Reference<DerivedMockObject> refDerived(derivedObject);
			{
				Reference<MockObject> ref;
				ref = refObject;
				EXPECT_TRUE(ref != nullptr);
				ref = refDerivedObject;
				EXPECT_TRUE(ref != nullptr);
				ref = refOther;
				EXPECT_TRUE(ref == nullptr);
				ref = refDerived;
				EXPECT_TRUE(ref != nullptr);
			}
			{
				Reference<OtherMockObject> ref;
				ref = refObject;
				EXPECT_TRUE(ref == nullptr);
				ref = refDerivedObject;
				EXPECT_TRUE(ref == nullptr);
				ref = refOther;
				EXPECT_TRUE(ref != nullptr);
				ref = refDerived;
				EXPECT_TRUE(ref == nullptr);
			}
			{
				Reference<DerivedMockObject> ref;
				ref = refObject;
				EXPECT_TRUE(ref == nullptr);
				ref = refDerivedObject;
				EXPECT_TRUE(ref != nullptr);
				ref = refOther;
				EXPECT_TRUE(ref == nullptr);
				ref = refDerived;
				EXPECT_TRUE(ref != nullptr);
			}
		}
	}
}

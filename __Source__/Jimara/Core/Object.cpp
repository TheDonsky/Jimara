#include "Object.h"
#include "BulkAllocated.h"


namespace Jimara {
#ifndef NDEBUG
	namespace {
		static std::atomic<std::size_t>& DEBUG_ActiveObjectCount() {
			static std::atomic<std::size_t> count = 0;
			return count;
		}
	}

	std::size_t Object::DEBUG_ActiveInstanceCount() {
		return DEBUG_ActiveObjectCount();
	}
#endif

	Object::Object() : m_referenceCount(1) { 
#ifndef NDEBUG
		DEBUG_ActiveObjectCount()++;
#endif
	}

	Object::~Object() { 
#ifndef NDEBUG
		DEBUG_ActiveObjectCount()--;
#endif
	}

	void Object::AddRef()const { m_referenceCount++; }

	void Object::ReleaseRef()const {
		std::size_t count = m_referenceCount.fetch_sub(1);
		if (count == 1) OnOutOfScope();
	}

	std::size_t Object::RefCount()const {
		return m_referenceCount;
	}

	void Object::OnOutOfScope()const {
		const BulkAllocated* bulkAllocated = dynamic_cast<const BulkAllocated*>(this);
		if (bulkAllocated == nullptr) {
			delete this;
			return;
		}
		Reference<BulkAllocated::BaseAllocationGroup> group = bulkAllocated->m_allocationGroup;
		bulkAllocated->m_allocationGroup = nullptr;
		if (group != nullptr)
			group->Deallocate(bulkAllocated);
		else delete this;
	}
}

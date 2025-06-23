#pragma once
#include "Object.h"
#include "Function.h"
#include "Synch/SpinLock.h"
#include <cassert>
#include <type_traits>


namespace Jimara {
	/// <summary>
	/// A generic object, that can be allocated in bulk
	/// </summary>
	class JIMARA_API BulkAllocated : public virtual Object {
	public:
		/// <summary>
		/// Bulk-allocated objects will typically be trying to have 16384 byte chunks.
		/// </summary>
		static const constexpr size_t ALLOCATION_BLOCK_REFERENCE_SIZE = 16384u;

		/// <summary>
		/// Number of elements allocated within the same block for given Bulk-Allocated type
		/// </summary>
		/// <typeparam name="ObjectType"> BulkAllocated type </typeparam>
		template<typename ObjectType>
		static const constexpr size_t BLOCK_ALLOCATION_COUNT =
			(ALLOCATION_BLOCK_REFERENCE_SIZE + sizeof(ObjectType) - 1u) / sizeof(ObjectType);

		/// <summary>
		/// Allocates instance of a bulk-allocated object
		/// </summary>
		/// <typeparam name="ObjectType"> A class derived from BulkAllocated </typeparam>
		/// <typeparam name="...ConstructorArgs"> Constructor argument types </typeparam>
		/// <param name="...args"> Constructor arguments (instance is instantiated as: new ObjectType(args...)) </param>
		/// <returns> New instance of a BulkAllocated object </returns>
		template<typename ObjectType, typename... ConstructorArgs>
		inline static std::enable_if_t<std::is_base_of_v<BulkAllocated, ObjectType>, Reference<ObjectType>> Allocate(ConstructorArgs... args);

		/// <summary> Constructor </summary>
		inline BulkAllocated() {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~BulkAllocated() {
			assert(m_allocationGroup == nullptr);
		}

	private:
		// Allocation-group:
		struct BaseAllocationGroup : public virtual Object {
			virtual ~BaseAllocationGroup() {}
			virtual void Deallocate(const BulkAllocated* instance) = 0;
		};
		mutable Reference<BaseAllocationGroup> m_allocationGroup = nullptr;

		// Object will free the allocation if needed (we could override OnOutOfScope, but this way we can stay more flexible)
		friend class Object;
	};



	/// <summary>
	/// Allocates instance of a bulk-allocated object
	/// </summary>
	/// <typeparam name="ObjectType"> A class derived from BulkAllocated </typeparam>
	/// <typeparam name="...ConstructorArgs"> Constructor argument types </typeparam>
	/// <param name="...args"> Constructor arguments (instance is instantiated as: new ObjectType(args...)) </param>
	/// <returns> New instance of a BulkAllocated object </returns>
	template<typename ObjectType, typename... ConstructorArgs>
	inline static std::enable_if_t<std::is_base_of_v<BulkAllocated, ObjectType>, Reference<ObjectType>> BulkAllocated::Allocate(ConstructorArgs... args) {
		static_assert(BLOCK_ALLOCATION_COUNT<ObjectType> > 0u);

		// Allocation-Block will contain memory for storing some amount of allocations (around 16kb):
		struct AllocationBlock : public virtual BaseAllocationGroup {
			uint8_t buffer[sizeof(ObjectType) * BLOCK_ALLOCATION_COUNT<ObjectType>];
			std::atomic_size_t allocationIndex = 0u;
			virtual ~AllocationBlock() {}

			// Bulk-allocated instance will need to invoke destructor and release allocation-block reference:
			virtual void Deallocate(const BulkAllocated* allocation) final override {
				assert(allocation->m_allocationGroup == nullptr);
				const ObjectType* instance = dynamic_cast<const ObjectType*>(allocation);
				assert(instance != nullptr);
				instance->~ObjectType();
			}
		};

		// Obtain allocation-block and unused slot:
		Reference<AllocationBlock> allocationBlock;
		size_t allocationIndex = 0u;
		while (true) {
			static thread_local Reference<AllocationBlock> currentAllocationBlock;
			if (currentAllocationBlock == nullptr)
				currentAllocationBlock = Object::Instantiate<AllocationBlock>();
			allocationBlock = currentAllocationBlock;
			allocationIndex = allocationBlock->allocationIndex.fetch_add(1u);
			if (allocationIndex < BLOCK_ALLOCATION_COUNT<ObjectType>)
				break;
			else currentAllocationBlock = nullptr;
		}

		// Invoke constructor and set allocation-group:
		ObjectType* instancePtr = reinterpret_cast<ObjectType*>((uint8_t*)allocationBlock->buffer) + allocationIndex;
		new(instancePtr)ObjectType(args...);
		instancePtr->m_allocationGroup = allocationBlock;

		// Create strong-reference, remove extra ref-count and return:
		Reference<ObjectType> res(instancePtr);
		instancePtr->ReleaseRef();
		return res;
	}

	static_assert(!std::is_base_of_v<BulkAllocated, Object>);
	static_assert(std::is_base_of_v<Object, BulkAllocated>);
	static_assert(sizeof(uint8_t) == 1u);
}

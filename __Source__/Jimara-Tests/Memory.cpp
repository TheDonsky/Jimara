#include "Memory.h"
#include <iostream>
#include <cassert>
#include <atomic>
#ifndef _WIN32
#ifdef __APPLE__
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif
#else
#include <windows.h>
#include <psapi.h>
#endif
#include "Core/Object.h"

namespace Jimara {
	namespace Test {
		namespace Memory {
			namespace {
				static std::atomic<std::size_t> ALLOCATION = 0;
				static std::atomic<std::size_t> TOTAL_ALLOCATION = 0;
				static std::atomic<std::size_t> TOTAL_DEALLOCATION = 0;
			}
		}
	}
}


#pragma warning(disable: 28196)
#pragma warning(disable: 28251)
#pragma warning(disable: 6387)

void* operator new(size_t size) {
	void* alloc = malloc(size);
	assert(alloc != nullptr);
#ifdef _WIN32
	size_t allocSize = _msize(alloc);
#elif __APPLE__
	size_t allocSize = malloc_size(alloc);
#else
	size_t allocSize = malloc_usable_size(alloc);
#endif
	Jimara::Test::Memory::ALLOCATION += allocSize;
	Jimara::Test::Memory::TOTAL_ALLOCATION += allocSize;
	return alloc;
}

void operator delete(void* alloc) {
	if (alloc != nullptr) {
#ifdef _WIN32
		size_t allocSize = _msize(alloc);
#elif __APPLE__
		size_t allocSize = malloc_size(alloc);
#else
		size_t allocSize = malloc_usable_size(alloc);
#endif
		Jimara::Test::Memory::TOTAL_DEALLOCATION += allocSize;
		Jimara::Test::Memory::ALLOCATION -= allocSize;
		free(alloc);
	}
}

#pragma warning(default: 28196)
#pragma warning(default: 28251)
#pragma warning(default: 6387)


namespace Jimara {
	namespace Test {
		namespace Memory {
			size_t HeapAllocation() {
				return ALLOCATION;
			}

			size_t TotalAllocation() {
				return TOTAL_ALLOCATION;
			}

			size_t TotalDeallocation() {
				return TOTAL_DEALLOCATION;
			}

			void LogMemoryState() {
				std::cout << "Heap: Current allocation-" << Jimara::Test::Memory::HeapAllocation()
					<< "; Total allocation-" << Jimara::Test::Memory::TotalAllocation()
					<< "; Total deallocation-" << Jimara::Test::Memory::TotalDeallocation() << std::endl;
			}

			MemorySnapshot::MemorySnapshot() :
#ifndef NDEBUG
				initialInstanceCount(Object::DEBUG_ActiveInstanceCount()),
#endif
				initialAllocation(Test::Memory::HeapAllocation()) {}

			bool MemorySnapshot::Compare()const {
#ifndef NDEBUG
				size_t currentInstanceCount = Object::DEBUG_ActiveInstanceCount();
				if (initialInstanceCount != currentInstanceCount) {
					std::cout << "MemorySnapshot::MemorySnapshot - "
						<< "Error: initialInstanceCount(" << initialInstanceCount << 
						") != currentInstanceCount(" << currentInstanceCount << "); either there are some new global objects or there might be a resource leak..." << std::endl;
					return false;
				}
#endif
				size_t currentHeapAllocation = Test::Memory::HeapAllocation();
				if (initialAllocation < currentHeapAllocation)
					std::cout << "MemorySnapshot::MemorySnapshot - " 
					<< "Warning: initialAllocation < currentHeapAllocation; either some driver has not freed memory yet or there might be a leak..." << std::endl;
				else if (initialAllocation > currentHeapAllocation)
					std::cout << "MemorySnapshot::MemorySnapshot - " 
					<< "Warning: initialAllocation > currentHeapAllocation; either some driver freed memory late or there might be a double free..." << std::endl;
				return true;
			}
		}
	}
}

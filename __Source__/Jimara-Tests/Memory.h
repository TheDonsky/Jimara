#pragma once
#include <cstddef>


namespace Jimara {
	namespace Test {
		namespace Memory {
			/// <summary> Current heap-allocated CPU memory </summary>
			size_t HeapAllocation();

			/// <summary> Total heap-allocated CPU memory </summary>
			size_t TotalAllocation();

			/// <summary> Current heap-deallocated CPU memory </summary>
			size_t TotalDeallocation();

			/// <summary> Reports HeapAllocation, TotalAllocation and TotalDeallocation on standard output </summary>
			void LogMemoryState();

			/// <summary>
			/// Tracks allocations between two or more points during execution
			/// Note: Useful for detecting possible memory leaks
			/// </summary>
			struct MemorySnapshot {
#ifndef NDEBUG
				/// <summary> Number of initial Object instances </summary>
				size_t initialInstanceCount;
#endif
				/// <summary> Initial HeapAllocation() </summary>
				size_t initialAllocation;

				/// <summary>
				/// Constructor
				/// initialInstanceCount = Object::DEBUG_ActiveInstanceCount();
				/// initialAllocation = HeapAllocation();
				/// </summary>
				MemorySnapshot();

				/// <summary> Compares current allocations to the snapshot and returns true, if they match </summary>
				bool Compare()const;
			};
		}
	}
}

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
		}
	}
}

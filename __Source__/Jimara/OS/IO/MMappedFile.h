#pragma once
#include "../../Core/Object.h"
#include <string>
#ifdef _WIN32
#include <windows.h>
#else
#endif 



namespace Jimara {
	namespace OS {
		/// <summary>
		/// __TODO__: Not implemented
		/// </summary>
		class MMappedFile : public virtual Object {
		public:
			MMappedFile(const std::wstring_view& filename, bool writeEnabled = false, size_t segmentStart = 0, size_t segmentSize = ~size_t(0));

			virtual ~MMappedFile();

			void* DataRW();

			const void* Data()const;

			size_t Size()const;

		private:
#ifdef _WIN32
#else
#endif 
		};
	}
}

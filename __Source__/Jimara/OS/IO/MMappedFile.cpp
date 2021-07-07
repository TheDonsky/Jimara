#include "MMappedFile.h"


namespace Jimara {
	namespace OS {
		MMappedFile::MMappedFile(const std::wstring_view& filename, bool writeEnabled, size_t segmentStart, size_t segmentSize) {
			// __TODO__: Implement this....
		}

		MMappedFile::~MMappedFile() {
			// __TODO__: Implement this....
		}

		void* MMappedFile::DataRW() {
			return nullptr;
			// __TODO__: Implement this....
		}

		const void* MMappedFile::Data()const {
			return nullptr;
			// __TODO__: Implement this....
		}

		size_t MMappedFile::Size()const {
			return 0;
			// __TODO__: Implement this....
		}
	}
}

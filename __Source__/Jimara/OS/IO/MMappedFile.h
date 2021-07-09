#pragma once
#include "../../Core/Memory/MemoryBlock.h"
#include "../Logging/Logger.h"




namespace Jimara {
	namespace OS {
		class MMappedFile : public virtual Object {
		public:
			static Reference<MMappedFile> Create(const std::string_view& filename, OS::Logger* logger = nullptr, bool cached = true);

			virtual operator MemoryBlock()const = 0;
		};
	}
}

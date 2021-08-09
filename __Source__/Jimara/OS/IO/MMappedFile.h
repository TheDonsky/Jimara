#pragma once
#include "../../Core/Memory/MemoryBlock.h"
#include "../Logging/Logger.h"




namespace Jimara {
	namespace OS {
		/// <summary>
		/// Memory-mapped file (read-only)
		/// </summary>
		class MMappedFile : public virtual Object {
		public:
			/// <summary>
			/// Memory-maps file
			/// </summary>
			/// <param name="filename"> Path to the file to be mapped </param>
			/// <param name="logger"> Logger for error reporting </param>
			/// <param name="cached"> If true, the memory mapping will be cached and, therefore, recreated if and only if there's no active mapping in the system </param>
			/// <returns> New file mapping if successuful, false otherwise </returns>
			static Reference<MMappedFile> Create(const std::string_view& filename, OS::Logger* logger = nullptr, bool cached = true);

			/// <summary>
			/// Type cast to MemoryBlock 
			/// (retains original mapped file reference, so this is safe to use even if you loose the reference to the mapping)
			/// </summary>
			virtual operator MemoryBlock()const = 0;
		};
	}
}

#pragma once
#include "../IO/Path.h"
#include "../Logging/Logger.h"


namespace Jimara {
	namespace OS {
		/// <summary>
		/// Loaded Dynamic Link Library (DLL[Windows]/SO[Linux]) module
		/// </summary>
		class DynamicLibrary : public virtual Object {
		public:
			/// <summary>
			/// Loads DLL from given path 
			/// <para/> Note that actual module will always be loaded once per file and retrieved from the cache on repeated Load() requests.
			/// </summary>
			/// <param name="path"> Path to the module (.dll/.so) </param>
			/// <param name="logger"> System logger for error reporting </param>
			/// <returns> Dynamic library module </returns>
			static Reference<DynamicLibrary> Load(const Path& path, Logger* logger);

		private:
			// OS-Specific implementation
			class Implementation;

			// Constructor is private..
			inline DynamicLibrary() {}
		};
	}
}

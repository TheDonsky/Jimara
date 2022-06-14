#include "DynamicLibrary.h"
#include "../../Core/Collections/ObjectCache.h"


namespace Jimara {
	namespace OS {
		class DynamicLibrary::Implementation : public virtual DynamicLibrary, public virtual ObjectCache<Path>::StoredObject {
		private:
#ifdef _WIN32
			// __TODO__: Implement this crap!
#else 
			// __TODO__: Implement this crap!
#endif

		public:
			class Cache : public virtual ObjectCache<Path> {
			public:
				inline static Reference<DynamicLibrary> Load(const Path& path, Logger* logger) {
					static Cache cache;
					return cache.GetCachedOrCreate(path, false, [&]() -> Reference<ObjectCache<Path>::StoredObject> {
						// __TODO__: Implement this crap!
						if (logger != nullptr) logger->Error("DynamicLibrary::Load - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						return nullptr;
						});
				}
			};
		};

		Reference<DynamicLibrary> DynamicLibrary::Load(const Path& path, Logger* logger) {
			Implementation::Cache::Load(path, logger);
		}
	}
}

#include "ShaderLoader.h"


namespace Jimara {
#pragma warning(disable: 4250)
	namespace {
		class Cache : public virtual Graphics::ShaderDirectory, public virtual ObjectCache<std::string>::StoredObject {
		public:
			inline Cache(OS::Logger* logger, const std::string& path) : Graphics::ShaderDirectory(path, logger) {}
		};
	}
#pragma warning(default: 4250)


	ShaderDirectoryLoader::ShaderDirectoryLoader(OS::Logger* logger) : m_logger(logger) {}

	Reference<Graphics::ShaderSet> ShaderDirectoryLoader::LoadShaderSet(const std::string& setIdentifier) {
		return GetCachedOrCreate(setIdentifier, false, [&]() ->Reference<Graphics::ShaderSet> {
			return Object::Instantiate<Cache>(m_logger, setIdentifier);
			});
	}
}

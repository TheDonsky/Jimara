#include "ShaderLoader.h"
#include <sstream>


namespace Jimara {
#pragma warning(disable: 4250)
	namespace {
		class Cache : public virtual Graphics::ShaderDirectory, public virtual ObjectCache<std::string>::StoredObject {
		public:
			inline Cache(OS::Logger* logger, const std::string& path) : Graphics::ShaderDirectory(path, logger) {}
		};
	}
#pragma warning(default: 4250)


	ShaderDirectoryLoader::ShaderDirectoryLoader(const std::string& baseDirectory, OS::Logger* logger) 
		: m_baseDirectory(baseDirectory), m_logger(logger) {}

	Reference<Graphics::ShaderSet> ShaderDirectoryLoader::LoadShaderSet(const std::string& setIdentifier) {
		return GetCachedOrCreate(setIdentifier, false, [&]() ->Reference<Graphics::ShaderSet> {
			std::stringstream stream;
			stream << m_baseDirectory;
			if (m_baseDirectory.length() > 0
				&& m_baseDirectory[m_baseDirectory.length() - 1] != '/'
				&& m_baseDirectory[m_baseDirectory.length() - 1] != '\\') stream << '/';
			stream << setIdentifier;
			return Object::Instantiate<Cache>(m_logger, stream.str());
			});
	}
}

#include "ShaderLoader.h"
#include <sstream>


namespace Jimara {
	namespace Graphics {
#pragma warning(disable: 4250)
		namespace {
			class Cache : public virtual ShaderDirectory, public virtual ObjectCache<OS::Path>::StoredObject {
			public:
				inline Cache(OS::Logger* logger, const OS::Path& path) : ShaderDirectory(path, logger) {}
			};
		}
#pragma warning(default: 4250)


		ShaderDirectoryLoader::ShaderDirectoryLoader(const OS::Path& baseDirectory, OS::Logger* logger)
			: m_baseDirectory(baseDirectory), m_logger(logger) {}

		Reference<ShaderSet> ShaderDirectoryLoader::LoadShaderSet(const OS::Path& setIdentifier) {
			return GetCachedOrCreate(setIdentifier, false, [&]() ->Reference<ShaderSet> {
				const std::wstring baseDirectory = m_baseDirectory;
				std::wstringstream stream;
				stream << m_baseDirectory;
				if (baseDirectory.length() > 0
					&& baseDirectory[baseDirectory.length() - 1] != L'/'
					&& baseDirectory[baseDirectory.length() - 1] != L'\\') stream << L'/';
				stream << setIdentifier;
				return Object::Instantiate<Cache>(m_logger, stream.str());
				});
		}
	}
}

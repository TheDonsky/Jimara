#pragma once
#include "../../../Graphics/Data/ShaderBinaries/ShaderSet.h"


namespace Jimara {
	class ShaderLoader : public virtual Object {
	public:
		virtual Reference<Graphics::ShaderSet> LoadShaderSet(const std::string& setIdentifier) = 0;
	};

	class ShaderDirectoryLoader : public virtual ShaderLoader, public virtual ObjectCache<std::string> {
	public:
		ShaderDirectoryLoader(OS::Logger* logger);

		virtual Reference<Graphics::ShaderSet> LoadShaderSet(const std::string& setIdentifier) override;

	private:
		const Reference<OS::Logger> m_logger;
	};
}

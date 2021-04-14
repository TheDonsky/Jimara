#pragma once
#include "../../OS/Logging/Logger.h"
#include <vector>

namespace Jimara {
	class ShaderBytecode : public virtual Object {
	public:
		ShaderBytecode(const char* data, size_t size, OS::Logger* logger);

		ShaderBytecode(const std::vector<char*>& data, OS::Logger* logger);

		ShaderBytecode(std::vector<char*>&& data, OS::Logger* logger);

		virtual ~ShaderBytecode();


	private:
		const Reference<OS::Logger> m_logger;
		const std::vector<char*> m_data;
	};
}

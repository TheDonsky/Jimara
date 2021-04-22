#pragma once
#include "../../../OS/Logging/Logger.h"
#include <vector>
#include <string>


namespace Jimara {
	namespace Graphics {
		class SPIRV_Binary : public virtual Object {
		public:
			static Reference<SPIRV_Binary> FromSPV(const std::string& filename, OS::Logger* logger);

			static Reference<SPIRV_Binary> FromData(const void* data, size_t size, OS::Logger* logger);

			static Reference<SPIRV_Binary> FromData(const std::vector<uint8_t>& data, OS::Logger* logger);

			static Reference<SPIRV_Binary> FromData(const std::vector<char>& data, OS::Logger* logger);

			virtual ~SPIRV_Binary();


		private:
			const Reference<OS::Logger> m_logger;

			SPIRV_Binary(const void* data, size_t size, OS::Logger* logger);
		};
	}
}

#pragma once
#include "ShaderClass.h"


namespace Jimara {
	namespace Graphics {
		class ShaderSet : public virtual Object {
		public:
			virtual Reference<SPIRV_Binary> GetShaderModule(ShaderClass* shaderClass, PipelineStage stage) = 0;
		};

		class ShaderDirectory : public virtual ShaderSet {
		public:
			ShaderDirectory(const std::string_view& directory, OS::Logger* logger);

			Reference<SPIRV_Binary> GetShaderModule(const std::string& shaderPath, PipelineStage stage);

			virtual Reference<SPIRV_Binary> GetShaderModule(ShaderClass* shaderClass, PipelineStage stage) override;

		private:
			const Reference<OS::Logger> m_logger;
			const std::string m_directory;
		};
	}
}

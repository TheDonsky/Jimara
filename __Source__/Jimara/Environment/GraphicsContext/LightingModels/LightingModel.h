#pragma once
#include "../GraphicsContext.h"


namespace Jimara {
	class LightingModel : public virtual Object {
	public:
		struct LitShaderModules {
			Reference<Graphics::SPIRV_Binary> vertexShader;
			Reference<Graphics::SPIRV_Binary> fragmentShader;
		};

		typedef std::string ShaderIdentifier;

		virtual Reference<Graphics::PipelineDescriptor> EnvironmentPipelineDescriptor() = 0;

		virtual LitShaderModules GetShaderBinaries(const ShaderIdentifier& shaderId) = 0;

		Reference<Graphics::GraphicsPipeline::Descriptor> CreatePipelineDescriptor(const SceneObjectDescriptor* descriptor);
	};
}

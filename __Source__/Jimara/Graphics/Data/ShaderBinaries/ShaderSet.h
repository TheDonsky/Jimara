#pragma once
#include "ShaderClass.h"


namespace Jimara {
	namespace Graphics {
		class ShaderSet : public virtual Object {
		public:
			virtual Reference<SPIRV_Binary> GetShaderModule(ShaderClass* shaderClass, PipelineStage stage) = 0;
		};
	}
}

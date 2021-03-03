#pragma once
#include "../Graphics/GraphicsDevice.h"


namespace Jimara {
	class Material : public virtual Graphics::PipelineDescriptor::BindingSetDescriptor {
	public:
		virtual Graphics::PipelineDescriptor::BindingSetDescriptor* EnvironmentDescriptor()const = 0;

		virtual Reference<Graphics::Shader> VertexShader()const = 0;

		virtual Reference<Graphics::Shader> FragmentShader()const = 0;
	};
}

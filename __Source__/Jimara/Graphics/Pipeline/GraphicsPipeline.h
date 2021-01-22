#pragma once
namespace Jimara {
	namespace Graphics {
		class GraphicsPipeline;
	}
}
#include "Shader.h"


namespace Jimara {
	namespace Graphics {
		class GraphicsPipeline : public virtual Object {
		public:
			class Descriptor : public virtual Object {
			public:
				virtual Reference<Shader> VertexShader() = 0;

				virtual Reference<Shader> FragmentShader() = 0;
			};
		};
	}
}

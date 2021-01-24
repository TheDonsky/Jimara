#pragma once
namespace Jimara {
	namespace Graphics {
		class GraphicsPipeline;
	}
}
#include "Shader.h"
#include "../Memory/Buffers.h"


namespace Jimara {
	namespace Graphics {
		class GraphicsPipeline : public virtual Object {
		public:
			class Descriptor : public virtual Object {
			public:
				virtual Reference<Shader> VertexShader() = 0;

				virtual Reference<Shader> FragmentShader() = 0;

				virtual size_t VertexBufferCount() = 0;

				virtual Reference<Graphics::VertexBuffer> VertexBuffer(size_t index) = 0;

				virtual size_t InstanceBufferCount() = 0;

				virtual Reference<Graphics::InstanceBuffer> InstanceBuffer(size_t index) = 0;

				virtual BufferArrayReference<uint32_t> IndexBuffer() = 0;

				virtual size_t IndexCount() = 0;

				virtual size_t InstanceCount() = 0;
			};
		};
	}
}

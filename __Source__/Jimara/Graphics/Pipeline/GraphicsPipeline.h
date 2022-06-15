#pragma once
namespace Jimara {
	namespace Graphics {
		class GraphicsPipeline;
	}
}
#include "Pipeline.h"


namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Pipeline that draws graphics to a frame buffer
		/// </summary>
		class JIMARA_API GraphicsPipeline : public virtual Pipeline {
		public:
			/// <summary>
			/// Type of the geometry primitives or index interpretation
			/// </summary>
			enum class IndexType : uint8_t {
				/// <summary> Indices are recognized as triplets of triangle vertices, rendering geometry as a triangle mesh </summary>
				TRIANGLE,

				/// <summary> Indices are recognized as couples of edge endpoints, rendering geometry as wireframe </summary>
				EDGE
			};

			/// <summary>
			/// Graphics pipeline descriptor
			/// </summary>
			class JIMARA_API Descriptor : public virtual PipelineDescriptor {
			public:
				/// <summary> Vertex shader </summary>
				virtual Reference<Shader> VertexShader()const = 0;

				/// <summary> Fragment shader </summary>
				virtual Reference<Shader> FragmentShader()const = 0;


				/// <summary> Number of vertex buffers, used by the vertex shader </summary>
				virtual size_t VertexBufferCount()const = 0;

				/// <summary>
				/// Vertex buffer by index
				/// </summary>
				/// <param name="index"> Vertex buffer index </param>
				/// <returns> Index'th vertex buffer </returns>
				virtual Reference<Graphics::VertexBuffer> VertexBuffer(size_t index) = 0;


				/// <summary> Number of instance buffers, used by the vertex shader (basically, vertex buffers that are delivered per-instance, instead of per vertex) </summary>
				virtual size_t InstanceBufferCount()const = 0;

				/// <summary>
				/// Instance buffer by index
				/// </summary>
				/// <param name="index"> Instance buffer index </param>
				/// <returns> Index'th instance buffer </returns>
				virtual Reference<Graphics::InstanceBuffer> InstanceBuffer(size_t index) = 0;


				/// <summary> Index buffer </summary>
				virtual ArrayBufferReference<uint32_t> IndexBuffer() = 0;

				/// <summary> Type of the geometry primitives or index interpretation (TRIANGLE(filled; multiples of 3) or EDGE(wireframe; pairs of 2)) </summary>
				virtual IndexType GeometryType() = 0;

				/// <summary> Number of indices to use from index buffer (helps when we want to reuse the index buffer object even when we change geometry or something) </summary>
				virtual size_t IndexCount() = 0;

				/// <summary> Number of instances to draw (by ignoring some of the instance buffer members, we can mostly vary instance count without any reallocation) </summary>
				virtual size_t InstanceCount() = 0;
			};
		};
	}
}

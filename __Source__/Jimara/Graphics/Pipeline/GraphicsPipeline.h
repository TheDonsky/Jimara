#pragma once
namespace Jimara {
	namespace Graphics {
		class GraphicsPipeline;
		class VertexBuffer;
	}
}
#include "../Data/ShaderBinaries/SPIRV_Binary.h"
#include "Pipeline.h"
#include "IndirectBuffers.h"


namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Vertex/Instance buffer interface
		/// </summary>
		class JIMARA_API VertexBuffer : public virtual Object {
		public:
			/// <summary> Buffer attribute description </summary>
			struct AttributeInfo {
				/// <summary> Attribute type </summary>
				SPIRV_Binary::ShaderInputInfo::Type type;

				/// <summary> glsl location </summary>
				uint32_t location;

				/// <summary> Attribute offset within buffer element in bytes </summary>
				size_t offset;
			};

			/// <summary> Virtual destructor </summary>
			inline virtual ~VertexBuffer() {}

			/// <summary> Number of attributes exposed from each buffer element </summary>
			virtual size_t AttributeCount()const = 0;

			/// <summary>
			/// Exposed buffer elemnt attribute by index
			/// </summary>
			/// <param name="index"> Attribute index </param>
			/// <returns> Attribute description </returns>
			virtual AttributeInfo Attribute(size_t index)const = 0;

			/// <summary>
			/// Size of an individual element within the buffer;
			/// (Actual Buffer() may change over lifetime, this one has to stay constant)
			/// </summary>
			virtual size_t BufferElemSize()const = 0;

			/// <summary> Buffer, carrying the vertex/instance data </summary>
			virtual Reference<ArrayBuffer> Buffer() = 0;
		};

		/// <summary> Vertex/Instance buffer interface </summary>
		typedef VertexBuffer InstanceBuffer;

		/// <summary>
		/// Pipeline that draws graphics to a frame buffer
		/// </summary>
		class JIMARA_API GraphicsPipeline : public virtual Pipeline {
		public:
			/// <summary>
			/// Type of the geometry primitives or index interpretation
			/// </summary>
			using IndexType = Experimental::GraphicsPipeline::IndexType;

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

				/// <summary>
				/// Indirect draw buffer
				/// <para/> Notes:
				///		<para/> 0. If not null, indirect index draw command will be used;
				///		<para/> 1. If provided, InstanceCount will be understood as indirect draw command count.
				/// </summary>
				virtual IndirectDrawBufferReference IndirectBuffer() = 0;

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

#pragma once
#include "Buffers.h"
#include "../../Core/EnumClassBooleanOperands.h"

namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Acceleration structure for Ray-Tracing
		/// </summary>
		class JIMARA_API AccelerationStructure : public virtual Object {
		public:
			/// <summary> General AS flags </summary>
			enum class JIMARA_API Flags : uint8_t {
				/// <summary> Empty bitmask </summary>
				NONE = 0u,

				/// <summary> 
				/// If set, additional memory will be allocated for potential updates that may be performed instead of full rebuilds 
				/// (updates require 'source' acceleration structure)
				/// </summary>
				ALLOW_UPDATES = (1u << 0u),

				/// <summary> 
				/// If set, this flag will tell the underlying API to prioritize build time over trace performance 
				/// (may come in handy whem there are frequent updates) 
				/// </summary>
				PREFER_FAST_BUILD = (1u << 1u),

				/// <summary> If set, this flag will guarantee that the any-hit shader will be invoked no more than once per primitive during a single trace </summary>
				PREVENT_DUPLICATE_ANY_HIT_INVOCATIONS = (1u << 2u)
			};

			/// <summary> Virtual destructore </summary>
			inline virtual ~AccelerationStructure() {}
		};
		
		// Flags can be concatenated
		JIMARA_DEFINE_ENUMERATION_BOOLEAN_OPERATIONS(AccelerationStructure::Flags);

		/// <summary>
		/// Bottom-Level Acceleration structure for Ray-Tracing
		/// <para/> Stores Geometry BVH itself;
		/// </summary>
		class JIMARA_API BottomLevelAccelerationStructure : public virtual AccelerationStructure {
		public:
			/// <summary> BLAS Vertex format </summary>
			enum class JIMARA_API VertexFormat {
				/// <summary> Vector3 (32 bit XYZ) </summary>
				X32Y32Z32 = 0u,

				/// <summary> Half3 (16 bit XYZ) </summary>
				X16Y16Z16 = 1u
			};

			/// <summary> BLAS Index buffer format </summary>
			enum class JIMARA_API IndexFormat {
				/// <summary> uint32_t (32 bit unsigned integers) </summary>
				U32 = 0u,

				/// <summary> uint16_t (16 bit unsigned integers) </summary>
				U16 = 1u
			};

			/// <summary>
			/// General Creation-Time propertis of the acceleration structure
			/// </summary>
			struct JIMARA_API Properties {
				/// <summary> Maximal number of triangles, the AS can contain </summary>
				uint32_t maxTriangleCount = 0u;

				/// <summary> Maximal number of vertices the stored geometry can contain </summary>
				uint32_t maxVertexCount = 0u;

				/// <summary> BLAS Vertex format </summary>
				VertexFormat vertexFormat = VertexFormat::X32Y32Z32;

				/// <summary> BLAS Index buffer format </summary>
				IndexFormat indexFormat = IndexFormat::U32;

				/// <summary> AS Flags </summary>
				Flags flags = Flags::NONE;
			};

			/// <summary> Virtual destructore </summary>
			inline virtual ~BottomLevelAccelerationStructure() {}

			/// <summary>
			/// Builds BLAS
			/// </summary>
			/// <param name="commandBuffer"> Command buffer to record to </param>
			/// <param name="vertexBuffer"> Vertex buffer </param>
			/// <param name="vertexStride"> Vertex buffer stride (can be different from the ObjectSize) </param>
			/// <param name="positionFieldOffset"> 
			/// Offset (in bytes) of the first vertex position record inside the vertexBuffer 
			/// <para/> Position format must be the same as the one from creation time 
			/// </param>
			/// <param name="indexBuffer"> 
			/// Index buffer 
			/// <para/> Format has to be the same as the one from creation-time;
			/// <para/> Buffer may not be of a correct type, but it will be treated as a blob containing nothing but indices.
			/// </param>
			/// <param name="updateSrcBlas"> 
			/// 'Source' acceleration structure for the update (versus rebuild)
			/// <para/> nullptr means rebuild request;
			/// <para/> Updates require ALLOW_UPDATES flag; if not provided during creation, this pointer will be ignored;
			/// <para/> updateSrcBlas can be the same as this, providing an option to update in-place.
			/// </param>
			/// <param name="vertexCount"> 
			/// Number of vertices (by default, full buffer will be 'consumed' after positionFieldOffset) 
			/// <para/> You can control first vertex index by manipulating positionFieldOffset.
			/// </param>
			/// <param name="indexCount"> Number of indices (HAS TO BE a multiple of 3; by default, full buffer is used) </param>
			/// <param name="firstIndex"> Index buffer offset (in indices, not bytes; 0 by default) </param>
			virtual void Build(
				CommandBuffer* commandBuffer,
				ArrayBuffer* vertexBuffer, size_t vertexStride, size_t positionFieldOffset,
				ArrayBuffer* indexBuffer,
				BottomLevelAccelerationStructure* updateSrcBlas = nullptr,
				size_t vertexCount = ~size_t(0u),
				size_t indexCount = ~size_t(0u),
				size_t firstIndex = 0u) = 0;
		};

		/// <summary>
		/// Top-Level Acceleration structure for Ray-Tracing
		/// <para/> Stores BottomLevelAccelerationStructure instances as BVH content;
		/// </summary>
		class JIMARA_API TopLevelAccelerationStructure : public virtual AccelerationStructure {
		public:
			/// <summary> Virtual destructore </summary>
			inline virtual ~TopLevelAccelerationStructure() {}
		};
	}
}

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

			/// <summary> Device address of the acceleration structure </summary>
			virtual uint64_t DeviceAddress()const = 0;
		};


		/// <summary> BLAS Instance descriptor </summary>
		struct JIMARA_API AccelerationStructureInstanceDesc {
			/// <summary> 3 by 4 transformation matrix </summary>
			glm::mat3x4 transform;

			/// <summary> 24-bit user-specified index value accessible to ray shaders in the InstanceCustomIndexKHR built-in </summary>
			uint32_t instanceCustomIndex : 24;

			/// <summary> An 8-bit visibility mask for the geometry. The instance may only be hit if Cull Mask & instance.mask != 0 </summary>
			uint32_t visibilityMask : 8;

			/// <summary> 24-bit offset used in calculating the hit shader binding table index </summary>
			uint32_t shaderBindingTableRecordOffset : 24;

			/// <summary> 8-bit flags required by underlying APIs. </summary>
			uint32_t instanceFlags : 8;

			/// <summary> 
			// DeviceAddress() of the bottom-level acceleration structure 
			// keep in mind, that blas HAS TO BE kept alive, while TLAS is in use) 
			// </summary>
			uint64_t blasDeviceAddress;

			/// <summary> Flags that can be used within instanceFlags </summary>
			enum class JIMARA_API Flags : uint8_t {
				/// <summary> Empty bitmask </summary>
				NONE = 0,

				/// <summary> Disables face culling for this instance </summary>
				DISABLE_BACKFACE_CULLING = 0x00000001,

				/// <summary> 
				/// According to Vulkan documentation:
				/// Specifies that the facing determination for geometry in this instance is inverted. 
				/// Because the facing is determined in object space, an instance transform does not change the winding, but a geometry transform does.
				/// </summary>
				FLIP_FACES = 0x00000002,

				/// <summary> Marks instance as opaque; Can be overriden during trace with an appropriate flag. </summary>
				FORCE_OPAQUE = 0x00000004
			};
		};

		// Define bitmask boolean operators for flags:
		JIMARA_DEFINE_ENUMERATION_BOOLEAN_OPERATIONS(AccelerationStructureInstanceDesc::Flags);


		/// <summary>
		/// Top-Level Acceleration structure for Ray-Tracing
		/// <para/> Stores BottomLevelAccelerationStructure instances as BVH content;
		/// </summary>
		class JIMARA_API TopLevelAccelerationStructure : public virtual AccelerationStructure {
		public:
			/// <summary>
			/// General Creation-Time propertis of the acceleration structure
			/// </summary>
			struct JIMARA_API Properties {
				/// <summary> Maximal number of bottom-level acceleration structure instances, the AS can contain </summary>
				uint32_t maxBottomLevelInstances = 0u;

				/// <summary> AS Flags </summary>
				Flags flags = Flags::NONE;
			};

			/// <summary> Virtual destructore </summary>
			inline virtual ~TopLevelAccelerationStructure() {}

			/// <summary>
			/// Builds TLAS
			/// <para/> Keep in mind that the user is FULLY responsible for keeping BLAS instances alive and valid while the TLAS is still in use,
			/// since the Top Level Acceleration structure does not copy their internal data and has no way to access their references.
			/// </summary>
			/// <param name="commandBuffer"> Command buffer to record to </param>
			/// <param name="instanceBuffer"> Buffer of contained BLAS instances
			/// <para/> Notes:
			/// <para/>		0. Used BLAS count has to be smaller than creation-time maxBottomLevelInstances value.
			/// <para/>		1. You can control the consumed range with instanceCount and firstInstance
			/// </param>
			/// <param name="updateSrcTlas">
			/// 'Source' acceleration structure for the update (versus rebuild)
			/// <para/> nullptr means rebuild request;
			/// <para/> Updates require ALLOW_UPDATES flag; if not provided during creation, this pointer will be ignored;
			/// <para/> updateSrcBlas can be the same as this, providing an option to update in-place.
			/// </param>
			/// <param name="instanceCount"> 
			/// Number of entries to place into the acceleration structure 
			/// <para/> Implementations will clamp the value to the minimal entry count after the firstInstance.
			/// </param>
			/// <param name="firstInstance"> 
			/// Index of the first entry to be taken into account from the instanceBuffer
			/// <para/> TLAS will be built using maximum of instanceCount instances from instanceBuffer starting from firstInstance.
			/// </param>
			virtual void Build(
				CommandBuffer* commandBuffer,
				const ArrayBufferReference<AccelerationStructureInstanceDesc>& instanceBuffer,
				TopLevelAccelerationStructure* updateSrcTlas = nullptr,
				size_t instanceCount = ~size_t(0u),
				size_t firstInstance = 0u) = 0;
		};
	}
}

#pragma once
#include "VulkanAccelerationStructure.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Vulkan-Backed BottomLevelAccelerationStructure
			/// </summary>
			class JIMARA_API VulkanBottomLevelAccelerationStructure 
				: public virtual VulkanAccelerationStructure
				, public virtual BottomLevelAccelerationStructure {
			public:
				/// <summary>
				/// Creates a BLAS
				/// </summary>
				/// <param name="device"> Graphics device </param>
				/// <param name="properties"> BLAS Settings </param>
				/// <returns> New BLAS instance </returns>
				static Reference<VulkanBottomLevelAccelerationStructure> Create(VulkanDevice* device, const BottomLevelAccelerationStructure::Properties& properties);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanBottomLevelAccelerationStructure();

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
					size_t firstIndex = 0u)override;


			private:
				// Create-time options:
				const BottomLevelAccelerationStructure::Properties m_properties;

				// Constructor
				VulkanBottomLevelAccelerationStructure(
					VkAccelerationStructureKHR accelerationStructure, VulkanArrayBuffer* buffer,
					const VkAccelerationStructureBuildSizesInfoKHR& buildSizes,
					const const BottomLevelAccelerationStructure::Properties& properties);

				// Private stuff...
				struct Helpers;
			};
		}
	}
}

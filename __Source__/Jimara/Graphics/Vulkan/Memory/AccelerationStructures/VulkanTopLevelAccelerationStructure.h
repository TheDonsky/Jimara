#pragma once
#include "VulkanAccelerationStructure.h"



namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Vulkan-Backed TopLevelAccelerationStructure
			/// </summary>
			class JIMARA_API VulkanTopLevelAccelerationStructure
				: public virtual VulkanAccelerationStructure
				, public virtual TopLevelAccelerationStructure {
			public:
				/// <summary>
				/// Creates a TLAS
				/// </summary>
				/// <param name="device"> Graphics device </param>
				/// <param name="properties"> TLAS Settings </param>
				/// <returns> New TLAS instance </returns>
				static Reference<VulkanTopLevelAccelerationStructure> Create(VulkanDevice* device, const TopLevelAccelerationStructure::Properties& properties);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanTopLevelAccelerationStructure();

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
					size_t firstInstance = 0u) override;

			private:
				// Create-time options:
				const TopLevelAccelerationStructure::Properties m_properties;

				// Constructor
				VulkanTopLevelAccelerationStructure(
					VkAccelerationStructureKHR accelerationStructure, VulkanArrayBuffer* buffer,
					const VkAccelerationStructureBuildSizesInfoKHR& buildSizes,
					const TopLevelAccelerationStructure::Properties& properties);

				// Private stuff...
				struct Helpers;
			};
		}
	}
}

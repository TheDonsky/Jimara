#pragma once
#include "../VulkanScratchBufferProvider.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class JIMARA_API VulkanBottomLevelAccelerationStructure;

			/// <summary>
			/// Vulkan-backed Acceleration structure
			/// </summary>
			class JIMARA_API VulkanAccelerationStructure : public virtual AccelerationStructure {
			public:
				/// <summary> Virtual destructor </summary>
				virtual ~VulkanAccelerationStructure();

				/// <summary>
				/// Translates AccelerationStructure::Flags to relevant VkBuildAccelerationStructureFlagsKHR bitmask
				/// </summary>
				/// <param name="flags"> AS Flags </param>
				/// <returns> VkBuildAccelerationStructureFlagsKHR </returns>
				static VkBuildAccelerationStructureFlagsKHR GetBuildFlags(AccelerationStructure::Flags flags);

				/// <summary>
				/// Translates AccelerationStructure::Flags to relevant VkGeometryFlagsKHR bitmask
				/// </summary>
				/// <param name="flags"> AS Flags </param>
				/// <returns> VkGeometryFlagsKHR </returns>
				static VkGeometryFlagsKHR GetGeometryFlags(AccelerationStructure::Flags flags);

				/// <summary> Device, the AS is allocated on </summary>
				inline VulkanDevice* Device()const { return m_buffer->Device(); }

				/// <summary> Type-cast to underlying resource </summary>
				inline operator VkAccelerationStructureKHR()const { return m_accelerationStructure; }

			private:
				// Underlying resource
				const VkAccelerationStructureKHR m_accelerationStructure;

				// "Owner" device
				const Reference<VulkanArrayBuffer> m_buffer;

				// Build sizes
				const VkAccelerationStructureBuildSizesInfoKHR m_buildSizes;

				// Scratch buffer provider
				const Reference<VulkanScratchBufferProvider> m_scratchBufferProvider;

				// Constructor is private (only limited concrete implementations can access it)
				VulkanAccelerationStructure(
					VkAccelerationStructureKHR accelerationStructure, VulkanArrayBuffer* buffer,
					const VkAccelerationStructureBuildSizesInfoKHR& buildSizes);

				// Retrieves scratch-buffer for the build commands
				inline Reference<VulkanArrayBuffer> GetScratchBuffer(bool update)const {
					return m_scratchBufferProvider->GetBuffer(update ? m_buildSizes.updateScratchSize : m_buildSizes.buildScratchSize);
				}

				// VulkanBottomLevelAccelerationStructure can access internals...
				friend class VulkanBottomLevelAccelerationStructure;
			};
		}
	}
}

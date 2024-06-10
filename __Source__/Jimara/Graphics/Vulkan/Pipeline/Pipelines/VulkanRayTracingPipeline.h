#pragma once
#include "VulkanPipeline.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
#pragma warning(disable: 4250)
			/// <summary>
			/// Vulkan-Backed RayTracingPipeline
			/// </summary>
			class JIMARA_API VulkanRayTracingPipeline : public virtual RayTracingPipeline, public virtual VulkanPipeline {
			public:
				/// <summary>
				/// Creates a Vulkan-backed Ray-Tracing pipeline instance
				/// </summary>
				/// <param name="device"> Graphics device </param>
				/// <param name="descriptor"> RT-Pipeline descriptor </param>
				/// <returns> New instance of a VulkanRayTracingPipeline </returns>
				static Reference<VulkanRayTracingPipeline> Create(VulkanDevice* device, const Descriptor& descriptor);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanRayTracingPipeline();

				/// <summary>
				/// Executes Ray-Tracing pipeline
				/// </summary>
				/// <param name="commandBuffer"> Command buffer to record commands to </param>
				/// <param name="kernelSize"> Kernel size (width, height and depth) </param>
				virtual void TraceRays(CommandBuffer* commandBuffer, const Size3& kernelSize) override;

			private:
				// Underlying pipeline object
				const VkPipeline m_pipeline;

				// SBT buffer
				const Reference<ArrayBuffer> m_bindingTable;

				// Ray-Gen shader region
				const VkStridedDeviceAddressRegionKHR m_rgenRegion = {};

				// Miss-Shader region
				const VkStridedDeviceAddressRegionKHR m_missRegion = {};

				// Hit-Group region
				const VkStridedDeviceAddressRegionKHR m_hitGroupRegion = {};

				// Callable-Shader region
				const VkStridedDeviceAddressRegionKHR m_callableRegion = {};

				// Constructor is private!
				VulkanRayTracingPipeline(BindingSetBuilder&& builder, 
					VkPipeline pipeline,
					ArrayBuffer* bindingTable,
					const VkStridedDeviceAddressRegionKHR& rgenRegion,
					const VkStridedDeviceAddressRegionKHR& missRegion,
					const VkStridedDeviceAddressRegionKHR& hitGroupRegion,
					const VkStridedDeviceAddressRegionKHR& callableRegion);
			};
#pragma warning(default: 4250)
		}
	}
}

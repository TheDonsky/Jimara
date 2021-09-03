#include "VulkanComputePipeline.h"
#include "VulkanShader.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanComputePipeline::VulkanComputePipeline(VulkanDevice* device, ComputePipeline::Descriptor* descriptor, size_t maxInFlightCommandBuffers) 
				: VulkanPipeline(device, descriptor, maxInFlightCommandBuffers), m_descriptor(descriptor), m_computePipeline(VK_NULL_HANDLE) {

				Reference<VulkanShader> shader = descriptor->ComputeShader();
				if (shader == nullptr) {
					device->Log()->Fatal("VulkanComputePipeline::VulkanComputePipeline - Vulkan shader module not provided!");
					return;
				}

				VkComputePipelineCreateInfo createInfo = {};
				{
					createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
					createInfo.pNext = nullptr;
					createInfo.flags = 0;
					
					createInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
					createInfo.stage.pNext = nullptr;
					createInfo.stage.flags = 0;
					createInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
					createInfo.stage.module = *shader;
					createInfo.stage.pName = "main";
					createInfo.stage.pSpecializationInfo = nullptr;

					createInfo.layout = PipelineLayout();
					createInfo.basePipelineHandle = VK_NULL_HANDLE;
					createInfo.basePipelineIndex = 0;
				}

				std::unique_lock<std::mutex> lock(Device()->PipelineCreationLock());
				VkPipeline pipeline;
				if (vkCreateComputePipelines(*Device(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline) != VK_SUCCESS) {
					device->Log()->Fatal("VulkanComputePipeline::VulkanComputePipeline - Failed to create compute pipeline!");
					return;
				}
				else m_computePipeline = pipeline;
			}

			VulkanComputePipeline::~VulkanComputePipeline() {
				if (m_computePipeline != VK_NULL_HANDLE) {
					std::unique_lock<std::mutex> lock(Device()->PipelineCreationLock());
					vkDestroyPipeline(*Device(), m_computePipeline, nullptr);
					m_computePipeline = VK_NULL_HANDLE;
				}
			}

			void VulkanComputePipeline::Execute(const CommandBufferInfo& bufferInfo) {
				VulkanCommandBuffer* commandBuffer = dynamic_cast<VulkanCommandBuffer*>(bufferInfo.commandBuffer);
				if (commandBuffer == nullptr) {
					Device()->Log()->Fatal("VulkanComputePipeline::Execute - Incompatible command buffer!");
					return;
				}

				Size3 kernelSize = m_descriptor->NumBlocks();
				if (kernelSize.x <= 0 || kernelSize.y <= 0 || kernelSize.z <= 0) return;

				VkMemoryBarrier barrier = {};
				{
					barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
					barrier.pNext = nullptr;
					barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
					barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				}

				vkCmdPipelineBarrier(*commandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
					| VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
				vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);

				UpdateDescriptors(bufferInfo);
				BindDescriptors(bufferInfo, VK_PIPELINE_BIND_POINT_COMPUTE);

				vkCmdDispatch(*commandBuffer, kernelSize.x, kernelSize.y, kernelSize.z);
				vkCmdPipelineBarrier(*commandBuffer, 
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
					| VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
					0, 1, &barrier, 0, nullptr, 0, nullptr);
				commandBuffer->RecordBufferDependency(this);
			}
		}
	}
}

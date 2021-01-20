#include "TriangleRenderer.h"
#include "Graphics/Vulkan/Pipeline/VulkanFrameBuffer.h"


#pragma warning(disable: 26812)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			namespace {
				class TriangleRendererData : public VulkanImageRenderer::EngineData {
				private:
					VkRenderPass m_renderPass;
					std::vector<Reference<VulkanFrameBuffer>> m_frameBuffers;

					TriangleRenderer* GetRenderer()const { return dynamic_cast<TriangleRenderer*>(Renderer()); }

					inline void CreateRenderPass() {
						VkAttachmentDescription attachment = {};
						{
							attachment.format = EngineInfo()->ImageFormat();
							attachment.samples = VK_SAMPLE_COUNT_1_BIT;

							attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
							attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

							attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
							attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

							attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
							attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
						}

						VkAttachmentReference reference = {};
						{
							reference.attachment = 0;
							reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
						}

						VkSubpassDescription subpass = {};
						{
							subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
							subpass.colorAttachmentCount = 1;
							subpass.pColorAttachments = &reference;
						}

						VkRenderPassCreateInfo renderPassInfo = {};
						{
							renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
							renderPassInfo.attachmentCount = 1;
							renderPassInfo.pAttachments = &attachment;
							renderPassInfo.subpassCount = 1;
							renderPassInfo.pSubpasses = &subpass;
						}
						if (vkCreateRenderPass(*EngineInfo()->VulkanDevice(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
							EngineInfo()->Log()->Fatal("TriangleRenderer - Failed to create render pass");
					}

				public:
					inline TriangleRendererData(TriangleRenderer* renderer, VulkanRenderEngineInfo* engineInfo) 
						: VulkanImageRenderer::EngineData(renderer, engineInfo), m_renderPass(VK_NULL_HANDLE) {
						CreateRenderPass();
						for (size_t i = 0; i < engineInfo->ImageCount(); i++) {
							VulkanImage* image = engineInfo->Image(i);
							if (image == nullptr)
								engineInfo->Log()->Error("TriangleRenderer - Image is null");
							else m_frameBuffers.push_back(Object::Instantiate<VulkanFrameBuffer>(
								std::vector<Reference<VulkanImageView>>{ Object::Instantiate<VulkanImageView>(image) }, m_renderPass));
						}
					}

					inline virtual ~TriangleRendererData() {
						if (m_renderPass != VK_NULL_HANDLE) {
							vkDestroyRenderPass(*EngineInfo()->VulkanDevice(), m_renderPass, nullptr);
							m_renderPass = VK_NULL_HANDLE;
						}
					}

					inline VkRenderPass RenderPass()const { return m_renderPass; }

					inline VulkanFrameBuffer* FrameBuffer(size_t imageId)const { return m_frameBuffers[imageId]; }
				};
			}
			
			TriangleRenderer::TriangleRenderer(VulkanDevice* device) 
				: m_device(device) {}

			Reference<VulkanImageRenderer::EngineData> TriangleRenderer::CreateEngineData(VulkanRenderEngineInfo* engineInfo) {
				return Object::Instantiate<TriangleRendererData>(this, engineInfo);
			}

			void TriangleRenderer::Render(EngineData* engineData, VulkanRenderEngine::CommandRecorder* commandRecorder) {
				TriangleRendererData* data = dynamic_cast<TriangleRendererData*>(engineData);
				assert(data != nullptr);
				VkCommandBuffer commandBuffer = commandRecorder->CommandBuffer();
				if (commandBuffer == VK_NULL_HANDLE)
					engineData->EngineInfo()->Log()->Fatal("TriangleRenderer - Command buffer not provided");

				// Begin render pass
				{
					VkRenderPassBeginInfo info = {};
					info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
					info.renderPass = data->RenderPass();
					info.framebuffer = *data->FrameBuffer(commandRecorder->ImageIndex());
					info.renderArea.offset = { 0, 0 };
					glm::uvec2 size = engineData->EngineInfo()->TargetSize();
					info.renderArea.extent = { size.x, size.y };
					VkClearValue clearValue = {};
					clearValue.color = { 0.0f, 0.25f, 0.25f, 1.0f };
					info.clearValueCount = 1;
					info.pClearValues = &clearValue;
					vkCmdBeginRenderPass(commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
				}

				// End render pass
				vkCmdEndRenderPass(commandBuffer);
			}
		}
	}
}
#pragma warning(default: 26812)

#include "VulkanRenderPass.h"
#include "VulkanFrameBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanGraphicsPipeline.h"
#include "Experimental/VulkanGraphicsPipeline_Exp.h"
#include "../../Pipeline/Experimental/LegacyPipelines.h"


#pragma warning(disable: 26812)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanRenderPass::VulkanRenderPass(
				VulkanDevice* device,
				Texture::Multisampling sampleCount,
				size_t numColorAttachments, const Texture::PixelFormat* colorAttachmentFormats,
				Texture::PixelFormat depthFormat,
				RenderPass::Flags flags)
				: RenderPass(flags, min(sampleCount, device->PhysicalDevice()->MaxMultisapling()), numColorAttachments, colorAttachmentFormats, depthFormat)
				, m_device(device), m_renderPass(VK_NULL_HANDLE) {

				static thread_local std::vector<VkAttachmentDescription2> attachments;
				static thread_local std::vector<VkAttachmentReference2> refs;
				attachments.clear();
				refs.clear();

				// Color attachments (indices: 0 to ColorAttachmentCount())
				const VkSampleCountFlagBits samples = m_device->PhysicalDeviceInfo()->SampleCountFlags(SampleCount());
				for (size_t i = 0; i < ColorAttachmentCount(); i++) {
					VkAttachmentDescription2 desc = {};
					desc.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
					
					desc.format = VulkanImage::NativeFormatFromPixelFormat(ColorAttachmentFormat(i));
					desc.samples = samples;

					desc.loadOp = ClearsColor() ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
					desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

					desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
					desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

					desc.initialLayout = ClearsColor() ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					desc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

					VkAttachmentReference2 ref = {};
					ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
					ref.attachment = static_cast<uint32_t>(attachments.size());
					ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

					attachments.push_back(desc);
					refs.push_back(ref);
				}

				// Resolve attachments (indices: ColorAttachmentCount() to 2 * m_colorAttachmentFormats.size())
				if (ResolvesColor()) for (size_t i = 0; i < ColorAttachmentCount(); i++) {
					VkAttachmentDescription2 desc = attachments[i];
					desc.samples = VK_SAMPLE_COUNT_1_BIT;
					desc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

					VkAttachmentReference2 ref = {};
					ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
					ref.attachment = static_cast<uint32_t>(attachments.size());
					ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

					attachments.push_back(desc);
					refs.push_back(ref);
				}

				// Depth attachment (index: ColorAttachmentCount() if there are no resolve attachments, 2 * ColorAttachmentCount() otherwise)
				if (HasDepthAttachment()) {
					VkAttachmentDescription2 desc = {};
					desc.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;

					desc.format = VulkanImage::NativeFormatFromPixelFormat(DepthAttachmentFormat());
					desc.samples = samples;

					desc.loadOp = ClearsDepth() ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
					desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

					desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
					desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

					desc.initialLayout = ClearsDepth() ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					desc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

					VkAttachmentReference2 ref = {};
					ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
					ref.attachment = static_cast<uint32_t>(attachments.size());
					ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

					attachments.push_back(desc);
					refs.push_back(ref);
				}

				// Depth resolve attachment
				VkSubpassDescriptionDepthStencilResolve depthResolve = {};
				if (ResolvesDepth()) {
					VkAttachmentDescription2 desc = attachments[DepthAttachmentId()];
					desc.samples = VK_SAMPLE_COUNT_1_BIT;
					desc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

					VkAttachmentReference2 ref = {};
					ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
					ref.attachment = static_cast<uint32_t>(attachments.size());
					ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

					attachments.push_back(desc);
					refs.push_back(ref);

					depthResolve.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;
					depthResolve.depthResolveMode = VK_RESOLVE_MODE_MIN_BIT;
					depthResolve.pDepthStencilResolveAttachment = &refs.back();
				}

				// Subpass:
				VkSubpassDescription2 subpass = {};
				{
					subpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
					subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
					subpass.colorAttachmentCount = static_cast<uint32_t>(ColorAttachmentCount());
					// The index of the attachment in this array is directly referenced from the fragment shader with the layout(location = 0) out vec4 outColor directive:
					subpass.pColorAttachments = refs.data();
					subpass.pDepthStencilAttachment = HasDepthAttachment() ? (refs.data() + DepthAttachmentId()) : nullptr;
					subpass.pResolveAttachments = ResolvesColor() ? (refs.data() + FirstResolveAttachmentId()) : nullptr;
					if (ResolvesDepth()) subpass.pNext = &depthResolve;
				}

				// Subpass dependencies:
				VkSubpassDependency2 dependency = {};
				{
					dependency.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
					dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
					dependency.dstSubpass = 0;
					dependency.srcStageMask = 
						VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
						VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
					dependency.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
					dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
					dependency.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				}

				// Render pass:
				VkRenderPassCreateInfo2 renderPassInfo = {};
				{
					renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
					renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
					renderPassInfo.pAttachments = attachments.data();
					renderPassInfo.subpassCount = 1;
					renderPassInfo.pSubpasses = &subpass;
					renderPassInfo.dependencyCount = 1;
					renderPassInfo.pDependencies = &dependency;
				}
				if (vkCreateRenderPass2(*m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
					m_renderPass = VK_NULL_HANDLE;
					m_device->Log()->Fatal("VulkanSwapChain - Failed to create render pass!");
				}
			}

			VulkanRenderPass::~VulkanRenderPass() {
				if (m_renderPass != VK_NULL_HANDLE) {
					vkDestroyRenderPass(*m_device, m_renderPass, nullptr);
					m_renderPass = VK_NULL_HANDLE;
				}
			}


			VulkanRenderPass::operator VkRenderPass()const {
				return m_renderPass;
			}

			size_t VulkanRenderPass::FirstColorAttachmentId()const {
				return 0;
			}

			size_t VulkanRenderPass::DepthAttachmentId()const {
				return ColorAttachmentCount() << (ResolvesColor() ? 1 : 0);
			}

			size_t VulkanRenderPass::FirstResolveAttachmentId()const { 
				return ColorAttachmentCount(); 
			}

			size_t VulkanRenderPass::DepthResolveAttachmentId()const {
				return DepthAttachmentId() + 1;
			}

			GraphicsDevice* VulkanRenderPass::Device()const {
				return m_device;
			}

			Reference<FrameBuffer> VulkanRenderPass::CreateFrameBuffer(
				Reference<TextureView>* colorAttachments, Reference<TextureView> depthAttachment,
				Reference<TextureView>* colorResolveAttachments, Reference<TextureView> depthResolveAttachment) {
				return Object::Instantiate<VulkanFrameBuffer>(this, colorAttachments, depthAttachment, colorResolveAttachments, depthResolveAttachment);
			}

			Reference<GraphicsPipeline> VulkanRenderPass::CreateGraphicsPipeline(GraphicsPipeline::Descriptor* descriptor, size_t maxInFlightCommandBuffers) {
				return Graphics::Experimental::LegacyGraphicsPipeline::Create(this, maxInFlightCommandBuffers, descriptor);
				//return Object::Instantiate<VulkanGraphicsPipeline>(descriptor, this, maxInFlightCommandBuffers);
			}

			Reference<Graphics::Experimental::GraphicsPipeline> VulkanRenderPass::GetGraphicsPipeline(
				const Graphics::Experimental::GraphicsPipeline::Descriptor& descriptor) {
				return Graphics::Vulkan::Experimental::VulkanGraphicsPipeline::Get(this, descriptor);
			}

			void VulkanRenderPass::BeginPass(CommandBuffer* commandBuffer, FrameBuffer* frameBuffer, const Vector4* clearValues, bool renderWithSecondaryCommandBuffers) {
				// Let's make sure, correct attachment types are provided
				VulkanCommandBuffer* vulkanBuffer = dynamic_cast<VulkanCommandBuffer*>(commandBuffer);
				VulkanFrameBuffer* vulkanFrame = dynamic_cast<VulkanFrameBuffer*>(frameBuffer);
				if (vulkanBuffer == nullptr || vulkanFrame == nullptr) {
					Device()->Log()->Fatal(std::string("VulkanRenderPass::BeginPass -")
						+ ((vulkanBuffer == nullptr) ? " Unsupported command buffer type;" : "")
						+ ((vulkanFrame == nullptr) ? " Unsupported frame buffer type;" : ""));
					return;
				}

				// Get static frame buffer handle
				vulkanBuffer->RecordBufferDependency(vulkanFrame);

				// Render pass begin info
				VkRenderPassBeginInfo renderPassInfo = {};
				{
					renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
					renderPassInfo.renderPass = m_renderPass;
					
					Size2 size = vulkanFrame->Resolution();
					renderPassInfo.framebuffer = *vulkanFrame;
					renderPassInfo.renderArea.offset = { 0, 0 };
					renderPassInfo.renderArea.extent = { size.x, size.y };
					
					static thread_local std::vector<VkClearValue> vulkanClearValueArgs;
					const size_t DEPTH_ATTACHMENT_ID = DepthAttachmentId();
					if (vulkanClearValueArgs.size() <= DEPTH_ATTACHMENT_ID)
						vulkanClearValueArgs.resize(DEPTH_ATTACHMENT_ID + 1);

					if (clearValues != nullptr) {
						for (size_t i = 0; i < ColorAttachmentCount(); i++) {
							const Vector4 clearColor = clearValues[i];
							vulkanClearValueArgs[i].color = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
						}
					}
					else for (size_t i = 0; i < ColorAttachmentCount(); i++)
						vulkanClearValueArgs[i].color = { 0.0f, 0.0f, 0.0f, 0.0f };

					vulkanClearValueArgs[DEPTH_ATTACHMENT_ID].depthStencil = { 1.0f, 0 };
					renderPassInfo.clearValueCount = static_cast<uint32_t>(DEPTH_ATTACHMENT_ID + 1);
					renderPassInfo.pClearValues = vulkanClearValueArgs.data();
				}

				// Set viewport & scisors
				{
					vkCmdSetScissor(*vulkanBuffer, 0, 1, &renderPassInfo.renderArea);
					VkViewport viewport = {};
					viewport.x = 0.0f;
					viewport.y = (float)renderPassInfo.renderArea.extent.height;
					viewport.width = (float)renderPassInfo.renderArea.extent.width;
					viewport.height = -viewport.y;
					viewport.minDepth = 0.0f;
					viewport.maxDepth = 1.0f;
					vkCmdSetViewport(*vulkanBuffer, 0, 1, &viewport);
				}

				// Begin render pass
				vkCmdBeginRenderPass(*vulkanBuffer, &renderPassInfo,
					renderWithSecondaryCommandBuffers ? VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS : VK_SUBPASS_CONTENTS_INLINE);
			}

			void VulkanRenderPass::EndPass(CommandBuffer* commandBuffer) {
				VulkanCommandBuffer* vulkanBuffer = dynamic_cast<VulkanCommandBuffer*>(commandBuffer);
				if (vulkanBuffer == nullptr)
					Device()->Log()->Fatal("VulkanRenderPass::EndPass - Unsupported command buffer type!");
				else vkCmdEndRenderPass(*vulkanBuffer);
			}
		}
	}
}
#pragma warning(default: 26812)

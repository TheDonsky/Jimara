#include "VulkanRenderPass.h"
#include "VulkanFrameBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanGraphicsPipeline.h"


#pragma warning(disable: 26812)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanRenderPass::VulkanRenderPass(
				VulkanDevice* device, Texture::Multisampling sampleCount
				, size_t numColorAttachments, Texture::PixelFormat* colorAttachmentFormats
				, Texture::PixelFormat depthFormat, bool includeResolveAttachments)
				: m_device(device), m_sampleCount(sampleCount)
				, m_colorAttachmentFormats(colorAttachmentFormats, colorAttachmentFormats + numColorAttachments)
				, m_depthAttachmentFormat(depthFormat), m_hasResolveAttachments(includeResolveAttachments)
				, m_renderPass(VK_NULL_HANDLE) {

				static thread_local std::vector<VkAttachmentDescription> attachments;
				static thread_local std::vector<VkAttachmentReference> refs;
				attachments.clear();
				refs.clear();

				// Color attachments (indices: 0 to m_colorAttachmentFormats.size())
				const VkSampleCountFlagBits samples = m_device->PhysicalDeviceInfo()->SampleCountFlags(m_sampleCount);
				for (size_t i = 0; i < m_colorAttachmentFormats.size(); i++) {
					VkAttachmentDescription desc = {};
					
					desc.format = VulkanImage::NativeFormatFromPixelFormat(m_colorAttachmentFormats[i]);
					desc.samples = samples;

					desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
					desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

					desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
					desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

					desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					desc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

					VkAttachmentReference ref = {};
					ref.attachment = static_cast<uint32_t>(attachments.size());
					ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

					attachments.push_back(desc);
					refs.push_back(ref);
				}

				// Resolve attachments (indices: m_colorAttachmentFormats.size() to 2 * m_colorAttachmentFormats.size())
				if (m_hasResolveAttachments) for (size_t i = 0; i < m_colorAttachmentFormats.size(); i++) {
					VkAttachmentDescription desc = attachments[0];
					desc.samples = VK_SAMPLE_COUNT_1_BIT;
					desc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

					VkAttachmentReference ref = {};
					ref.attachment = static_cast<uint32_t>(attachments.size());
					ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

					attachments.push_back(desc);
					refs.push_back(ref);
				}

				// Depth attachment (last index)
				if (HasDepthAttachment()) {
					VkAttachmentDescription desc = {};
					desc.format = VulkanImage::NativeFormatFromPixelFormat(m_depthAttachmentFormat);
					desc.samples = samples;

					desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
					desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

					desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
					desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

					desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					desc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

					VkAttachmentReference ref = {};
					ref.attachment = static_cast<uint32_t>(attachments.size());
					ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

					attachments.push_back(desc);
					refs.push_back(ref);
				}

				// Subpass:
				VkSubpassDescription subpass = {};
				{
					subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
					subpass.colorAttachmentCount = static_cast<uint32_t>(m_colorAttachmentFormats.size());
					// The index of the attachment in this array is directly referenced from the fragment shader with the layout(location = 0) out vec4 outColor directive:
					subpass.pColorAttachments = refs.data();
					subpass.pDepthStencilAttachment = HasDepthAttachment() ? &refs.back() : nullptr;
					subpass.pResolveAttachments = m_hasResolveAttachments ? (refs.data() + m_colorAttachmentFormats.size()) : nullptr;
				}

				// Subpass dependencies:
				VkSubpassDependency dependency = {};
				{
					dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
					dependency.dstSubpass = 0;
					dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
					dependency.srcAccessMask = 0;
					dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
					dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				}

				// Render pass:
				VkRenderPassCreateInfo renderPassInfo = {};
				{
					renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
					renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
					renderPassInfo.pAttachments = attachments.data();
					renderPassInfo.subpassCount = 1;
					renderPassInfo.pSubpasses = &subpass;
					renderPassInfo.dependencyCount = 1;
					renderPassInfo.pDependencies = &dependency;
				}
				if (vkCreateRenderPass(*m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
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

			Texture::Multisampling VulkanRenderPass::Multisampling()const {
				return m_sampleCount;
			}

			size_t VulkanRenderPass::ColorAttachmentCount()const {
				return m_colorAttachmentFormats.size();
			}

			Texture::PixelFormat VulkanRenderPass::ColorAttachmentFormat(size_t imageId)const {
				return m_colorAttachmentFormats[imageId];
			}

			size_t VulkanRenderPass::FirstColorAttachmentId()const {
				return 0;
			}

			bool VulkanRenderPass::HasDepthAttachment()const {
				return (m_depthAttachmentFormat >= Texture::PixelFormat::FIRST_DEPTH_FORMAT && m_depthAttachmentFormat <= Texture::PixelFormat::LAST_DEPTH_FORMAT);
			}

			Texture::PixelFormat VulkanRenderPass::DepthAttachmentFormat()const {
				return m_depthAttachmentFormat;
			}

			size_t VulkanRenderPass::DepthAttachmentId()const {
				return m_colorAttachmentFormats.size() << 1;
			}

			bool VulkanRenderPass::HasResolveAttachments()const {
				return m_hasResolveAttachments;
			}

			size_t VulkanRenderPass::FirstResolveAttachmentId()const {
				return m_colorAttachmentFormats.size();
			}

			GraphicsDevice* VulkanRenderPass::Device()const {
				return m_device;
			}

			Reference<FrameBuffer> VulkanRenderPass::CreateFrameBuffer(
				Reference<TextureView>* colorAttachments, Reference<TextureView> depthAttachment, Reference<TextureView>* resolveAttachments) {
				return Object::Instantiate<VulkanDynamicFrameBuffer>(this, colorAttachments, depthAttachment, resolveAttachments);
			}

			Reference<GraphicsPipeline> VulkanRenderPass::CreateGraphicsPipeline(GraphicsPipeline::Descriptor* descriptor, size_t maxInFlightCommandBuffers) {
				return Object::Instantiate<VulkanGraphicsPipeline>(descriptor, this, maxInFlightCommandBuffers);
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
				Reference<VulkanStaticFrameBuffer> staticFrame = vulkanFrame->GetStaticHandle(vulkanBuffer);
				if (staticFrame == vulkanFrame) vulkanBuffer->RecordBufferDependency(vulkanFrame);

				// Render pass begin info
				VkRenderPassBeginInfo renderPassInfo = {};
				{
					renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
					renderPassInfo.renderPass = m_renderPass;
					
					Size2 size = vulkanFrame->Resolution();
					renderPassInfo.framebuffer = *staticFrame;
					renderPassInfo.renderArea.offset = { 0, 0 };
					renderPassInfo.renderArea.extent = { size.x, size.y };
					
					static thread_local std::vector<VkClearValue> vulkanClearValueArgs;
					const size_t DEPTH_ATTACHMENT_ID = DepthAttachmentId();
					if (vulkanClearValueArgs.size() <= DEPTH_ATTACHMENT_ID)
						vulkanClearValueArgs.resize(DEPTH_ATTACHMENT_ID + 1);
					for (size_t i = 0; i < m_colorAttachmentFormats.size(); i++) {
						const Vector4 clearColor = clearValues[i];
						vulkanClearValueArgs[i].color = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
					}
					vulkanClearValueArgs[DEPTH_ATTACHMENT_ID].depthStencil = { 1.0f, 0 };
					renderPassInfo.clearValueCount = static_cast<uint32_t>(DEPTH_ATTACHMENT_ID + 1);
					renderPassInfo.pClearValues = vulkanClearValueArgs.data();
				}

				// Set viewport & scisors
				{
					vkCmdSetScissor(*vulkanBuffer, 0, 1, &renderPassInfo.renderArea);
					VkViewport viewport = {};
					viewport.x = viewport.y = 0;
					viewport.width = (float)renderPassInfo.renderArea.extent.width;
					viewport.height = (float)renderPassInfo.renderArea.extent.height;
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

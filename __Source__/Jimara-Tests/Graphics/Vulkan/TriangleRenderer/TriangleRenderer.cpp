#include "TriangleRenderer.h"
#include "Graphics/Vulkan/Pipeline/VulkanGraphicsPipeline.h"
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
					BufferArrayReference<uint32_t> m_indexBuffer;
					Reference<VulkanGraphicsPipeline> m_renderPipeline;

					TriangleRenderer* GetRenderer()const { return dynamic_cast<TriangleRenderer*>(Renderer()); }

					class Descriptor : public virtual GraphicsPipeline::Descriptor, public virtual VulkanGraphicsPipeline::RendererContext {
					private:
						TriangleRendererData* m_data;

					public:
						inline Descriptor(TriangleRendererData* data) : m_data(data) {}

						inline virtual Reference<Shader> VertexShader() override {
							return m_data->GetRenderer()->ShaderCache()->GetShader("Shaders/TriangleRenderer.vert.spv", false);
						}

						inline virtual Reference<Shader> FragmentShader() override {
							return m_data->GetRenderer()->ShaderCache()->GetShader("Shaders/TriangleRenderer.frag.spv", true);
						}

						inline virtual size_t VertexBufferCount() override {
							return 1;
						}

						inline virtual Reference<Graphics::VertexBuffer> VertexBuffer(size_t index) override {
							return m_data->GetRenderer()->PositionBuffer();
						}

						inline virtual size_t InstanceBufferCount() override {
							return 1;
						}

						inline virtual Reference<Graphics::InstanceBuffer> InstanceBuffer(size_t index) override {
							return m_data->GetRenderer()->InstanceOffsetBuffer();
						}

						inline virtual BufferArrayReference<uint32_t> IndexBuffer() override {
							return m_data->m_indexBuffer;
						}

						inline virtual size_t IndexCount() override {
							return m_data->GetRenderer()->PositionBuffer()->Buffer()->ObjectCount();
						}

						inline virtual size_t InstanceCount() override {
							return m_data->GetRenderer()->InstanceOffsetBuffer()->Buffer()->ObjectCount();
						}


						inline virtual VulkanDevice* Device() override {
							return dynamic_cast<VulkanDevice*>(m_data->EngineInfo()->Device());
						}

						inline virtual VkRenderPass RenderPass() override {
							return m_data->m_renderPass;
						}

						inline virtual VkSampleCountFlagBits TargetSampleCount() override {
							return VK_SAMPLE_COUNT_1_BIT;
						}

						inline virtual bool HasDepthAttachment() override {
							return false;
						}

						inline virtual Size2 TargetSize() override {
							return m_data->EngineInfo()->TargetSize();
						}
					} m_pipelineDescriptor;


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
						: VulkanImageRenderer::EngineData(renderer, engineInfo), m_renderPass(VK_NULL_HANDLE), m_pipelineDescriptor(this) {
						CreateRenderPass();
						for (size_t i = 0; i < engineInfo->ImageCount(); i++) {
							VulkanImage* image = engineInfo->Image(i);
							if (image == nullptr)
								engineInfo->Log()->Error("TriangleRenderer - Image is null");
							else m_frameBuffers.push_back(Object::Instantiate<VulkanFrameBuffer>(
								std::vector<Reference<VulkanImageView>>{ Object::Instantiate<VulkanImageView>(image) }, m_renderPass));
						}

						m_renderPipeline = Object::Instantiate<VulkanGraphicsPipeline>(&m_pipelineDescriptor, &m_pipelineDescriptor);
					}

					inline virtual ~TriangleRendererData() {
						m_renderPipeline = nullptr;
						if (m_renderPass != VK_NULL_HANDLE) {
							vkDestroyRenderPass(*EngineInfo()->VulkanDevice(), m_renderPass, nullptr);
							m_renderPass = VK_NULL_HANDLE;
						}
					}

					inline VkRenderPass RenderPass()const { return m_renderPass; }

					inline VulkanFrameBuffer* FrameBuffer(size_t imageId)const { return m_frameBuffers[imageId]; }

					inline VulkanGraphicsPipeline* Pipeline()const { return m_renderPipeline; }
				};
			}
			
			TriangleRenderer::TriangleRenderer(VulkanDevice* device)
				: m_device(device), m_shaderCache(device->CreateShaderCache())
				, m_positionBuffer(device), m_instanceOffsetBuffer(device) {}

			Reference<VulkanImageRenderer::EngineData> TriangleRenderer::CreateEngineData(VulkanRenderEngineInfo* engineInfo) {
				return Object::Instantiate<TriangleRendererData>(this, engineInfo);
			}

			Graphics::ShaderCache* TriangleRenderer::ShaderCache()const {
				return m_shaderCache;
			}

			void TriangleRenderer::Render(EngineData* engineData, VulkanRenderEngine::CommandRecorder* commandRecorder) {
				TriangleRendererData* data = dynamic_cast<TriangleRendererData*>(engineData);
				assert(data != nullptr);
				VkCommandBuffer commandBuffer = commandRecorder->CommandBuffer();
				if (commandBuffer == VK_NULL_HANDLE)
					engineData->EngineInfo()->Log()->Fatal("TriangleRenderer - Command buffer not provided");

				// Update pipeline buffers if there's a need to
				data->Pipeline()->UpdateBindings(commandRecorder);

				// Begin render pass
				{
					VkRenderPassBeginInfo info = {};
					info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
					info.renderPass = data->RenderPass();
					info.framebuffer = *data->FrameBuffer(commandRecorder->ImageIndex());
					info.renderArea.offset = { 0, 0 };
					Size2 size = engineData->EngineInfo()->TargetSize();
					info.renderArea.extent = { size.x, size.y };
					VkClearValue clearValue = {};
					clearValue.color = { 0.0f, 0.25f, 0.25f, 1.0f };
					info.clearValueCount = 1;
					info.pClearValues = &clearValue;
					vkCmdBeginRenderPass(commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
				}

				// Draw triangle
				data->Pipeline()->Render(commandRecorder);

				// End render pass
				vkCmdEndRenderPass(commandBuffer);
			}

			VertexBuffer* TriangleRenderer::PositionBuffer() {
				return &m_positionBuffer;
			}

			InstanceBuffer* TriangleRenderer::InstanceOffsetBuffer() {
				return &m_instanceOffsetBuffer;
			}


			TriangleRenderer::VertexPositionBuffer::VertexPositionBuffer(GraphicsDevice* device) {
				m_buffer = device->CreateArrayBuffer<Vector2>(6);
				Vector2* positions = m_buffer.Map();
				positions[0] = Vector2(-0.5f, -0.25f);
				positions[1] = Vector2(-0.25f, -0.75f);
				positions[2] = Vector2(-0.75f, -0.75f);
				positions[3] = Vector2(-0.5f, 0.25f);
				positions[4] = Vector2(-0.75f, 0.75f);
				positions[5] = Vector2(-0.25f, 0.75f);
				m_buffer->Unmap(true);
			}

			Reference<ArrayBuffer> TriangleRenderer::VertexPositionBuffer::Buffer()const {
				return m_buffer;
			}

			size_t TriangleRenderer::VertexPositionBuffer::AttributeCount()const {
				return 1;
			}

			VertexBuffer::AttributeInfo TriangleRenderer::VertexPositionBuffer::Attribute(size_t index)const {
				AttributeInfo info = {};
				{
					info.location = 0;
					info.offset = 0;
					info.type = AttributeInfo::Type::FLOAT2;
				}
				return info;
			}


			TriangleRenderer::InstanceOffsetBuffer::InstanceOffsetBuffer(GraphicsDevice* device) {
				m_buffer = device->CreateArrayBuffer<Vector2>(2);
				Vector2* positions = m_buffer.Map();
				positions[0] = Vector2(0.0f, 0.0f);
				positions[1] = Vector2(1.0f, 0.15f);
				m_buffer->Unmap(true);
			}

			Reference<ArrayBuffer> TriangleRenderer::InstanceOffsetBuffer::Buffer()const {
				return m_buffer;
			}

			size_t TriangleRenderer::InstanceOffsetBuffer::AttributeCount()const {
				return 1;
			}

			VertexBuffer::AttributeInfo TriangleRenderer::InstanceOffsetBuffer::Attribute(size_t index)const {
				AttributeInfo info = {};
				{
					info.location = 1;
					info.offset = 0;
					info.type = AttributeInfo::Type::FLOAT2;
				}
				return info;
			}
		}
	}
}
#pragma warning(default: 26812)

#include "VulkanGraphicsPipeline.h"
#include "VulkanShader.h"

#pragma warning(disable: 26812)

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			namespace {
				inline static VkPipelineLayout CreateVulkanPipelineLayout(VulkanGraphicsPipeline::RendererContext* context) {
					VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
					{
						pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
						pipelineLayoutInfo.setLayoutCount = 0; // __TODO__: Take inputs into consideration...
						pipelineLayoutInfo.pushConstantRangeCount = 0;
					}
					VkPipelineLayout pipelineLayout;
					if (vkCreatePipelineLayout(*context->Device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
						context->Device()->Log()->Fatal("VulkanRenderPipeline - Failed to create pipeline layout!");
						return VK_NULL_HANDLE;
					}
					return pipelineLayout;
				}

				inline static VkPipeline CreateVulkanPipeline(VulkanGraphicsPipeline::RendererContext* context, GraphicsPipeline::Descriptor* descriptor, VkPipelineLayout layout) {
					// ShaderStageInfos:
					VkPipelineShaderStageCreateInfo shaderStages[2] = { {}, {} };

					Reference<VulkanShader> vertexShader = descriptor->VertexShader();
					if (vertexShader != nullptr) {
						VkPipelineShaderStageCreateInfo& vertShaderStageInfo = shaderStages[0];
						vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
						vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
						vertShaderStageInfo.module = *vertexShader;
						vertShaderStageInfo.pName = "main";
					}
					else context->Device()->Log()->Fatal("VulkanRenderPipeline - Can not create render pipeline without vulkan shader module for Vertex shader!");

					Reference<VulkanShader> fragmentShader = descriptor->FragmentShader();
					if (fragmentShader != nullptr) {
						VkPipelineShaderStageCreateInfo& fragShaderStageInfo = shaderStages[1];
						fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
						fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
						fragShaderStageInfo.module = *fragmentShader;
						fragShaderStageInfo.pName = "main";
					}
					else context->Device()->Log()->Fatal("VulkanRenderPipeline - Can not create render pipeline without vulkan shader module for Fragment shader!");


					// Vertex input: <__TODO__>
					VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
					{
						vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
						vertexInputInfo.vertexBindingDescriptionCount = 0;//1;
						vertexInputInfo.pVertexBindingDescriptions = nullptr; //&BindingDecription();
						vertexInputInfo.vertexAttributeDescriptionCount = 0; //static_cast<uint32_t>(AttributeDescriptions().size());
						vertexInputInfo.pVertexAttributeDescriptions = nullptr; // AttributeDescriptions().data();
					}


					// Input assembly:
					VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
					{
						inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
						inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Triangle from every 3 vertices without reuse...
						inputAssembly.primitiveRestartEnable = VK_FALSE;
					}


					// Viewports and scisors: <__TODO__: Investigate further...>
					VkRect2D scissor = {};
					{
						scissor.offset = { 0, 0 };
						glm::uvec2 targetSize = context->TargetSize();
						scissor.extent = { targetSize.x, targetSize.y };
					}
					VkViewport viewport = {};
					{
						viewport.x = 0.0f;
						viewport.y = 0.0f;
						viewport.width = (float)scissor.extent.width;
						viewport.height = (float)scissor.extent.height;
						viewport.minDepth = 0.0f;
						viewport.maxDepth = 1.0f;
					}
					VkPipelineViewportStateCreateInfo viewportState = {};
					{
						viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
						viewportState.viewportCount = 1;
						viewportState.pViewports = &viewport;
						viewportState.scissorCount = 1;
						viewportState.pScissors = &scissor;
					}


					// Rasterizer: <__TODO__: Investigate further...>
					VkPipelineRasterizationStateCreateInfo rasterizer = {};
					{
						rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
						rasterizer.depthClampEnable = VK_FALSE;

						rasterizer.rasterizerDiscardEnable = VK_FALSE;

						rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

						rasterizer.lineWidth = 1.0f; // Superresolution? Maybe? Dunno....

						rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
						rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

						rasterizer.depthBiasEnable = VK_FALSE;
						rasterizer.depthBiasConstantFactor = 0.0f; // Optional
						rasterizer.depthBiasClamp = 0.0f; // Optional
						rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
					}


					// Multisampling:
					VkPipelineMultisampleStateCreateInfo multisampling = {};
					{
						multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
						multisampling.sampleShadingEnable = VK_FALSE; // DISABLING THIS ONE GIVES BETTER PERFORMANCE
						multisampling.rasterizationSamples = context->TargetSampleCount();
						multisampling.minSampleShading = 1.0f; // Optional
						multisampling.pSampleMask = nullptr; // Optional
						multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
						multisampling.alphaToOneEnable = VK_FALSE; // Optional
					}


					// Depth stencil state:
					VkPipelineDepthStencilStateCreateInfo depthStencil = {};
					{
						depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
						depthStencil.depthTestEnable = VK_TRUE;
						depthStencil.depthWriteEnable = VK_TRUE;

						depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

						depthStencil.depthBoundsTestEnable = VK_FALSE;
						depthStencil.minDepthBounds = 0.0f; // Optional
						depthStencil.maxDepthBounds = 1.0f; // Optional

						depthStencil.stencilTestEnable = VK_FALSE;
						depthStencil.front = {}; // Optional
						depthStencil.back = {}; // Optional
					}


					// Color blending:
					VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
					{
						colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
						colorBlendAttachment.blendEnable = VK_FALSE;
						colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
						colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
						colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
						colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
						colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
						colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
					}

					VkPipelineColorBlendStateCreateInfo colorBlending = {};
					{
						colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
						colorBlending.logicOpEnable = VK_FALSE;
						colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
						colorBlending.attachmentCount = 1;
						colorBlending.pAttachments = &colorBlendAttachment;
						colorBlending.blendConstants[0] = 0.0f; // Optional
						colorBlending.blendConstants[1] = 0.0f; // Optional
						colorBlending.blendConstants[2] = 0.0f; // Optional
						colorBlending.blendConstants[3] = 0.0f; // Optional
					}


					// Dynamic state:
					/*
					VkDynamicState dynamicStates[2] = {
						VK_DYNAMIC_STATE_VIEWPORT,
						VK_DYNAMIC_STATE_LINE_WIDTH
					};
					VkPipelineDynamicStateCreateInfo dynamicState = {};
					{
						dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
						dynamicState.dynamicStateCount = sizeof(dynamicStates) / sizeof(VkDynamicState);
						dynamicState.pDynamicStates = dynamicStates;
					}
					//*/


					// GRAPHICS PIPELINE:
					VkGraphicsPipelineCreateInfo pipelineInfo = {};
					{
						pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
						pipelineInfo.stageCount = 2;
						pipelineInfo.pStages = shaderStages; // Provides vertex and fragment shaders.
						pipelineInfo.pVertexInputState = &vertexInputInfo; // Defines the nature of vertex input (per-vertex data).
						pipelineInfo.pInputAssemblyState = &inputAssembly; // Tells pipeline to render triangles based on consecutive vertice triplets.
						pipelineInfo.pViewportState = &viewportState; // Tells the pipeline what area of the frame buffer to render to.
						pipelineInfo.pRasterizationState = &rasterizer; // Tells the rasterizer to fill in the triangle, cull backfaces and treat clockwise order as front face.
						pipelineInfo.pMultisampleState = &multisampling; // We are not exactly multisampling as of now... But this would be the place to define the damn thing.
						pipelineInfo.pDepthStencilState = context->HasDepthAttachment() ? &depthStencil : nullptr; // This tells to check depth...
						pipelineInfo.pColorBlendState = &colorBlending; // Forgot already... Has something to do with how we treat overlapping fragment colors.
						//pipelineInfo.pDynamicState = &dynamicState; // Defines, what aspects of our pipeline can change during runtime.
						pipelineInfo.layout = layout; // Pretty sure this is important as hell.
						pipelineInfo.renderPass = context->RenderPass(); // Pretty sure this is important as hell.
						pipelineInfo.subpass = 0; // Index of the subpass, out pipeline will be used at (Yep, the render pass so far consists of a single pass)
						pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
						pipelineInfo.basePipelineIndex = -1; // Optional
					}

					VkPipeline graphicsPipeline;
					if (vkCreateGraphicsPipelines(*context->Device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
						context->Device()->Log()->Fatal("TriangleTest - Failed to create graphics pipeline!");
						return VK_NULL_HANDLE;
					}
					else return graphicsPipeline;
				}
			}

			VulkanGraphicsPipeline::VulkanGraphicsPipeline(RendererContext* context, GraphicsPipeline::Descriptor* descriptor)
				: m_context(context), m_descriptor(descriptor)
				, m_pipelineLayout(VK_NULL_HANDLE), m_graphicsPipeline(VK_NULL_HANDLE) {
				m_pipelineLayout = CreateVulkanPipelineLayout(m_context);
				m_graphicsPipeline = CreateVulkanPipeline(m_context, m_descriptor, m_pipelineLayout);
			}

			VulkanGraphicsPipeline::~VulkanGraphicsPipeline() {
				vkDeviceWaitIdle(*m_context->Device());
				if (m_graphicsPipeline != VK_NULL_HANDLE) {
					vkDestroyPipeline(*m_context->Device(), m_graphicsPipeline, nullptr);
					m_graphicsPipeline = VK_NULL_HANDLE;
				}
				if (m_pipelineLayout != VK_NULL_HANDLE) {
					vkDestroyPipelineLayout(*m_context->Device(), m_pipelineLayout, nullptr);
					m_pipelineLayout = VK_NULL_HANDLE;
				}
			}

			void VulkanGraphicsPipeline::Render(VulkanRenderEngine::CommandRecorder* recorder) {
				VkCommandBuffer commandBuffer = recorder->CommandBuffer();
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
				vkCmdDraw(commandBuffer, 3, 1, 0, 0); // TMP
			}
		}
	}
}
#pragma warning(default: 26812)

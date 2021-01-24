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

				inline static VkPipeline CreateVulkanPipeline(
					VulkanGraphicsPipeline::RendererContext* context, GraphicsPipeline::Descriptor* descriptor, VkPipelineLayout layout
					, std::vector<Reference<VulkanDeviceResidentBuffer>>& vertexBuffers) {
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
					static thread_local std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions;
					static thread_local std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
					{
						vertexInputBindingDescriptions.clear();
						vertexInputAttributeDescriptions.clear();
						vertexBuffers.clear();

						auto addVertexBuffer = [&](Reference<VertexBuffer> vertexBuffer, VkVertexInputRate inputRate) {
							Reference<VulkanDeviceResidentBuffer> buffer = vertexBuffer->Buffer();
							if (buffer == nullptr) {
								context->Device()->Log()->Fatal("VulkanRenderPipeline - A vertex buffer of an incorrect type provided!");
								return;
							}

							vertexBuffers.push_back(buffer);
							
							VkVertexInputBindingDescription bindingDescription = {};
							{	
								bindingDescription.binding = static_cast<uint32_t>(vertexInputBindingDescriptions.size());
								bindingDescription.stride = static_cast<uint32_t>(buffer->ObjectSize());
								bindingDescription.inputRate = inputRate;
								vertexInputBindingDescriptions.push_back(bindingDescription);
							}

							const size_t attributeCount = vertexBuffer->AttributeCount();
							for (size_t i = 0; i < attributeCount; i++) {
								VertexBuffer::AttributeInfo attribute = vertexBuffer->Attribute(i);
								VkVertexInputAttributeDescription attributeDescription = {};
								attributeDescription.location = attribute.location;
								attributeDescription.binding = bindingDescription.binding;

								if (attribute.type >= VertexBuffer::AttributeInfo::Type::TYPE_COUNT) {
									context->Device()->Log()->Fatal("VulkanGraphicsPipeline - A vertex attribute with incorrect format provided");
									continue;
								}
								static const VkFormat* BINDING_TYPE_TO_FORMAT = []() -> VkFormat* {
									const uint8_t BINDING_TYPE_COUNT = static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::TYPE_COUNT);
									static VkFormat bindingTypeToFormats[BINDING_TYPE_COUNT];
									for (size_t i = 0; i < BINDING_TYPE_COUNT; i++) bindingTypeToFormats[i] = VK_FORMAT_MAX_ENUM;

									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::FLOAT)] = VK_FORMAT_R32_SFLOAT;
									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::FLOAT2)] = VK_FORMAT_R32G32_SFLOAT;
									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::FLOAT3)] = VK_FORMAT_R32G32B32_SFLOAT;
									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::FLOAT4)] = VK_FORMAT_R32G32B32A32_SFLOAT;

									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::INT)] = VK_FORMAT_R32_SINT;
									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::INT2)] = VK_FORMAT_R32G32_SINT;
									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::INT3)] = VK_FORMAT_R32G32B32_SINT;
									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::INT4)] = VK_FORMAT_R32G32B32A32_SINT;

									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::UINT)] = VK_FORMAT_R32_UINT;
									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::UINT2)] = VK_FORMAT_R32G32_UINT;
									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::UINT3)] = VK_FORMAT_R32G32B32_UINT;
									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::UINT4)] = VK_FORMAT_R32G32B32A32_UINT;

									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::BOOL32)] = VK_FORMAT_R32_UINT;

									return bindingTypeToFormats;
								}();
								attributeDescription.format = BINDING_TYPE_TO_FORMAT[static_cast<uint8_t>(attribute.type)];
								
								attributeDescription.offset = static_cast<uint32_t>(attribute.offset);

								// __TODO__: Check and make sure, this crap works...
								if (attributeDescription.format == VK_FORMAT_MAX_ENUM) {
									size_t numAdditions = 0;
									uint32_t offsetDelta = 0;
									if (attribute.type == VertexBuffer::AttributeInfo::Type::MAT_2X2) {
										attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
										numAdditions = 2;
										Matrix2 mat;
										offsetDelta = static_cast<uint32_t>(((char*)(&mat[0])) - ((char*)(&mat)));
									}
									else if (attribute.type == VertexBuffer::AttributeInfo::Type::MAT_3X3) {
										attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
										numAdditions = 3;
										Matrix3 mat;
										offsetDelta = static_cast<uint32_t>(((char*)(&mat[0])) - ((char*)(&mat)));
									}
									else if (attribute.type == VertexBuffer::AttributeInfo::Type::MAT_4X4) {
										attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
										numAdditions = 4;
										Matrix4 mat;
										offsetDelta = static_cast<uint32_t>(((char*)(&mat[0])) - ((char*)(&mat)));
									}
									else {
										context->Device()->Log()->Fatal("VulkanGraphicsPipeline - A vertex attribute with unknown format provided");
										continue;
									}
									for (size_t i = 0; i < numAdditions; i++) {
										vertexInputAttributeDescriptions.push_back(attributeDescription);
										attributeDescription.offset += offsetDelta;
										attributeDescription.location++;
									}
								}
								else vertexInputAttributeDescriptions.push_back(attributeDescription);
							}
						};

						const size_t vertexBufferCount = descriptor->VertexBufferCount();
						for (size_t bindingId = 0; bindingId < vertexBufferCount; bindingId++)
							addVertexBuffer(descriptor->VertexBuffer(bindingId), VK_VERTEX_INPUT_RATE_VERTEX);

						const size_t instanceBufferCount = descriptor->InstanceBufferCount();
						for (size_t bindingId = 0; bindingId < instanceBufferCount; bindingId++)
							addVertexBuffer(descriptor->InstanceBuffer(bindingId), VK_VERTEX_INPUT_RATE_INSTANCE);
					}
					VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
					{
						vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
						vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindingDescriptions.size());
						vertexInputInfo.pVertexBindingDescriptions = vertexInputBindingDescriptions.data();
						vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributeDescriptions.size());
						vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();
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
						Size2 targetSize = context->TargetSize();
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
						context->Device()->Log()->Fatal("VulkanGraphicsPipeline - Failed to create graphics pipeline!");
						return VK_NULL_HANDLE;
					}
					else return graphicsPipeline;
				}
			}

			VulkanGraphicsPipeline::VulkanGraphicsPipeline(RendererContext* context, GraphicsPipeline::Descriptor* descriptor)
				: m_context(context), m_descriptor(descriptor)
				, m_pipelineLayout(VK_NULL_HANDLE), m_graphicsPipeline(VK_NULL_HANDLE)
				, m_indexDataBuffer(VK_NULL_HANDLE) {
				m_pipelineLayout = CreateVulkanPipelineLayout(m_context);
				m_graphicsPipeline = CreateVulkanPipeline(m_context, m_descriptor, m_pipelineLayout, m_vertexBuffers);
				m_indexBuffer = m_descriptor->IndexBuffer();
				m_indexCount = static_cast<uint32_t>(m_descriptor->IndexCount());
				m_instanceCount = static_cast<uint32_t>(m_descriptor->InstanceCount());
				if (m_indexBuffer == nullptr && m_indexCount > 0) {
					BufferArrayReference<uint32_t> indexBuffer = ((GraphicsDevice*)m_context->Device())->CreateArrayBuffer<uint32_t>(m_indexCount);
					{
						uint32_t* indices = indexBuffer.Map();
						for (uint32_t i = 0; i < m_indexCount; i++)
							indices[i] = i;
						indexBuffer->Unmap(true);
					}
					m_indexBuffer = indexBuffer;
					if (m_indexBuffer == nullptr)
						m_context->Device()->Log()->Fatal("VulkanGraphicsPipeline - Internal error: Could not create proper index buffer!");
				}
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

			void VulkanGraphicsPipeline::UpdateBindings(VulkanRenderEngine::CommandRecorder* recorder) {
				// __TODO__: Update descriptors...

				const size_t vertexBufferCount = m_vertexBuffers.size();
				if (vertexBufferCount > 0) {
					if (m_vertexBindings.size() < vertexBufferCount) {
						m_vertexBindings.resize(vertexBufferCount);
						while (m_vertexBindingOffsets.size() < vertexBufferCount)
							m_vertexBindingOffsets.push_back(0);
					}
					for (size_t i = 0; i < vertexBufferCount; i++)
						m_vertexBindings[i] = *m_vertexBuffers[i]->GetDataBuffer(recorder);
				}
				m_indexDataBuffer = *m_indexBuffer->GetDataBuffer(recorder);
			}

			void VulkanGraphicsPipeline::Render(VulkanRenderEngine::CommandRecorder* recorder) {
				VkCommandBuffer commandBuffer = recorder->CommandBuffer();
				if (m_indexCount <= 0 || m_instanceCount <= 0) return;

				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
				
				// __TODO__: Bind descriptors...

				const size_t vertexBufferCount = m_vertexBindings.size();
				if (vertexBufferCount > 0)
					vkCmdBindVertexBuffers(commandBuffer, 0, static_cast<uint32_t>(vertexBufferCount), m_vertexBindings.data(), m_vertexBindingOffsets.data());
				
				if (m_indexDataBuffer != nullptr) {
					vkCmdBindIndexBuffer(commandBuffer, m_indexDataBuffer, 0, VK_INDEX_TYPE_UINT32);
					vkCmdDrawIndexed(commandBuffer, m_indexCount, m_instanceCount, 0, 0, 0);
				}
			}
		}
	}
}
#pragma warning(default: 26812)

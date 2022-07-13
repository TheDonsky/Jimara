#include "VulkanGraphicsPipeline.h"
#include "VulkanShader.h"

#pragma warning(disable: 26812)

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			namespace {
				inline static VkPipeline CreateVulkanPipeline(GraphicsPipeline::Descriptor* descriptor, VulkanRenderPass* renderPass, VkPipelineLayout layout) {
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
					else renderPass->Device()->Log()->Fatal("VulkanRenderPipeline - Can not create render pipeline without vulkan shader module for Vertex shader!");

					Reference<VulkanShader> fragmentShader = descriptor->FragmentShader();
					if (fragmentShader != nullptr) {
						VkPipelineShaderStageCreateInfo& fragShaderStageInfo = shaderStages[1];
						fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
						fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
						fragShaderStageInfo.module = *fragmentShader;
						fragShaderStageInfo.pName = "main";
					}
					else renderPass->Device()->Log()->Fatal("VulkanRenderPipeline - Can not create render pipeline without vulkan shader module for Fragment shader!");


					// Vertex input:
					static thread_local std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions;
					static thread_local std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
					{
						vertexInputBindingDescriptions.clear();
						vertexInputAttributeDescriptions.clear();
						
						auto addVertexBuffer = [&](Reference<const VertexBuffer> vertexBuffer, VkVertexInputRate inputRate) {
							VkVertexInputBindingDescription bindingDescription = {};
							{	
								bindingDescription.binding = static_cast<uint32_t>(vertexInputBindingDescriptions.size());
								bindingDescription.stride = static_cast<uint32_t>(vertexBuffer->BufferElemSize());
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
									renderPass->Device()->Log()->Fatal("VulkanGraphicsPipeline - A vertex attribute with incorrect format provided");
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

								if (attributeDescription.format == VK_FORMAT_MAX_ENUM) {
									size_t numAdditions = 0;
									uint32_t offsetDelta = 0;
									if (attribute.type == VertexBuffer::AttributeInfo::Type::MAT_2X2) {
										attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
										numAdditions = 2;
										Matrix2 mat;
										offsetDelta = static_cast<uint32_t>(((char*)(&mat[1])) - ((char*)(&mat)));
									}
									else if (attribute.type == VertexBuffer::AttributeInfo::Type::MAT_3X3) {
										attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
										numAdditions = 3;
										Matrix3 mat;
										offsetDelta = static_cast<uint32_t>(((char*)(&mat[1])) - ((char*)(&mat)));
									}
									else if (attribute.type == VertexBuffer::AttributeInfo::Type::MAT_4X4) {
										attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
										numAdditions = 4;
										Matrix4 mat;
										offsetDelta = static_cast<uint32_t>(((char*)(&mat[1])) - ((char*)(&mat)));
									}
									else {
										renderPass->Device()->Log()->Fatal("VulkanGraphicsPipeline - A vertex attribute with unknown format provided");
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
					const GraphicsPipeline::IndexType indexType = descriptor->GeometryType();
					VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
					{
						inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
						inputAssembly.topology =
							(indexType == GraphicsPipeline::IndexType::TRIANGLE) ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST : // Triangle from every 3 vertices without reuse...
							(indexType == GraphicsPipeline::IndexType::EDGE) ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST : // Edges from every 2 vertices without reuse...
							VK_PRIMITIVE_TOPOLOGY_POINT_LIST; // Just the points...
						inputAssembly.primitiveRestartEnable = VK_FALSE;
					}


					// Viewports state:
					VkPipelineViewportStateCreateInfo viewportState = {};
					{
						viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
						viewportState.viewportCount = 1;
						viewportState.scissorCount = 1;
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
						multisampling.rasterizationSamples = dynamic_cast<VulkanDevice*>(renderPass->Device())->PhysicalDeviceInfo()->SampleCountFlags(renderPass->SampleCount());
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

						depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

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
					static thread_local std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
					while (colorBlendAttachments.size() < renderPass->ColorAttachmentCount())
						colorBlendAttachments.push_back(colorBlendAttachment);

					VkPipelineColorBlendStateCreateInfo colorBlending = {};
					{
						colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
						colorBlending.logicOpEnable = VK_FALSE;
						colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
						colorBlending.attachmentCount = static_cast<uint32_t>(renderPass->ColorAttachmentCount());
						colorBlending.pAttachments = colorBlendAttachments.data();
						colorBlending.blendConstants[0] = 0.0f; // Optional
						colorBlending.blendConstants[1] = 0.0f; // Optional
						colorBlending.blendConstants[2] = 0.0f; // Optional
						colorBlending.blendConstants[3] = 0.0f; // Optional
					}


					// Dynamic state:
					VkDynamicState dynamicStates[2] = {
						VK_DYNAMIC_STATE_VIEWPORT,
						VK_DYNAMIC_STATE_SCISSOR
					};
					VkPipelineDynamicStateCreateInfo dynamicState = {};
					{
						dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
						dynamicState.dynamicStateCount = sizeof(dynamicStates) / sizeof(VkDynamicState);
						dynamicState.pDynamicStates = dynamicStates;
					}


					// GRAPHICS PIPELINE:
					VkGraphicsPipelineCreateInfo pipelineInfo = {};
					{
						pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
						pipelineInfo.stageCount = 2;
						pipelineInfo.pStages = shaderStages; // Provides vertex and fragment shaders.
						pipelineInfo.pVertexInputState = &vertexInputInfo; // Defines the nature of vertex input (per-vertex data).
						pipelineInfo.pInputAssemblyState = &inputAssembly; // Tells pipeline to render triangles based on consecutive vertice triplets or lines for wireframe.
						pipelineInfo.pViewportState = &viewportState; // Tells the pipeline what area of the frame buffer to render to.
						pipelineInfo.pRasterizationState = &rasterizer; // Tells the rasterizer to fill in the triangle, cull backfaces and treat clockwise order as front face (if index type is TRIANGLE).
						pipelineInfo.pMultisampleState = &multisampling; // We are not exactly multisampling as of now... But this would be the place to define the damn thing.
						pipelineInfo.pDepthStencilState = renderPass->HasDepthAttachment() ? &depthStencil : nullptr; // This tells to check depth...
						pipelineInfo.pColorBlendState = &colorBlending; // Forgot already... Has something to do with how we treat overlapping fragment colors.
						pipelineInfo.pDynamicState = &dynamicState; // Defines, what aspects of our pipeline can change during runtime.
						pipelineInfo.layout = layout; // Pretty sure this is important as hell.
						pipelineInfo.renderPass = *renderPass; // Pretty sure this is important as hell.
						pipelineInfo.subpass = 0; // Index of the subpass, out pipeline will be used at (Yep, the render pass so far consists of a single pass)
						pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
						pipelineInfo.basePipelineIndex = -1; // Optional
					}

					std::unique_lock<std::mutex> lock(dynamic_cast<VulkanDevice*>(renderPass->Device())->PipelineCreationLock());
					VkPipeline graphicsPipeline;
					if (vkCreateGraphicsPipelines(*dynamic_cast<VulkanDevice*>(renderPass->Device()), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
						renderPass->Device()->Log()->Fatal("VulkanGraphicsPipeline - Failed to create graphics pipeline!");
						return VK_NULL_HANDLE;
					}
					else return graphicsPipeline;
				}
			}

			VulkanGraphicsPipeline::VulkanGraphicsPipeline(GraphicsPipeline::Descriptor* descriptor, VulkanRenderPass* renderPass, size_t maxInFlightCommandBuffers)
				: VulkanPipeline(dynamic_cast<VulkanDevice*>(renderPass->Device()), descriptor, maxInFlightCommandBuffers), m_descriptor(descriptor), m_renderPass(renderPass)
				, m_graphicsPipeline(VK_NULL_HANDLE) {
				m_graphicsPipeline = CreateVulkanPipeline(m_descriptor, m_renderPass, PipelineLayout());
			}

			VulkanGraphicsPipeline::~VulkanGraphicsPipeline() {
				std::unique_lock<std::mutex> lock(dynamic_cast<VulkanDevice*>(m_renderPass->Device())->PipelineCreationLock());
				if (m_graphicsPipeline != VK_NULL_HANDLE) {
					vkDestroyPipeline(*dynamic_cast<VulkanDevice*>(m_renderPass->Device()), m_graphicsPipeline, nullptr);
					m_graphicsPipeline = VK_NULL_HANDLE;
				}
			}

			void VulkanGraphicsPipeline::Execute(const CommandBufferInfo& bufferInfo) {
				VulkanCommandBuffer* commandBuffer = dynamic_cast<VulkanCommandBuffer*>(bufferInfo.commandBuffer);
				if (commandBuffer == nullptr) m_renderPass->Device()->Log()->Fatal("VulkanGraphicsPipeline::Execute - Incompatible command buffer!");

				const uint32_t INDEX_COUNT = static_cast<uint32_t>(m_descriptor->IndexCount());

				// Update index buffer binding:
				if (INDEX_COUNT > 0) {
					Reference<VulkanArrayBuffer> indexBuffer = m_descriptor->IndexBuffer();
					if (indexBuffer != nullptr)
						m_indexBuffer = indexBuffer;
					else if (m_indexBuffer == nullptr || m_indexBuffer->ObjectCount() < INDEX_COUNT) {
						ArrayBufferReference<uint32_t> buffer = ((GraphicsDevice*)m_renderPass->Device())->CreateArrayBuffer<uint32_t>(INDEX_COUNT);
						{
							uint32_t* indices = buffer.Map();
							for (uint32_t i = 0; i < INDEX_COUNT; i++)
								indices[i] = i;
							buffer->Unmap(true);
						}
						m_indexBuffer = buffer;
					}
					commandBuffer->RecordBufferDependency(m_indexBuffer);
				}
				else return;


				// No rendering is necessary if there are no instances to speak of:
				const uint32_t INSTANCE_COUNT = static_cast<uint32_t>(m_descriptor->InstanceCount());
				if (INSTANCE_COUNT <= 0) return;

				static thread_local std::vector<VulkanArrayBuffer*> vertexBuffers;
				static thread_local std::vector<VkBuffer> vertexBindings;
				static thread_local std::vector<VkDeviceSize> vertexBindingOffsets;

				const uint32_t VERTEX_BUFFER_COUNT = static_cast<uint32_t>(m_descriptor->VertexBufferCount());
				const uint32_t INSTANCE_BUFFER_COUNT = static_cast<uint32_t>(m_descriptor->InstanceBufferCount());
				const uint32_t VERTEX_BINDING_COUNT = VERTEX_BUFFER_COUNT + VERTEX_BUFFER_COUNT;

				// Update vertex bindings:
				if (VERTEX_BINDING_COUNT > 0) {

					if (vertexBuffers.size() < VERTEX_BINDING_COUNT) {
						vertexBuffers.resize(VERTEX_BINDING_COUNT, nullptr);
						vertexBindings.resize(VERTEX_BINDING_COUNT, VK_NULL_HANDLE);
						vertexBindingOffsets.resize(VERTEX_BINDING_COUNT, 0);
					}

					size_t index = 0;
					auto addBuffer = [&](Reference<VulkanArrayBuffer> buffer) {
						VulkanArrayBuffer*& reference = vertexBuffers[index];
						if (buffer != nullptr) {
							reference = buffer;
							commandBuffer->RecordBufferDependency(buffer);
							vertexBindings[index] = *reference;
						}
						else {
							reference = nullptr;
							vertexBindings[index] = VK_NULL_HANDLE;
						}
						index++;
					};

					// Vertex buffers:
					for (size_t bindingId = 0; bindingId < VERTEX_BUFFER_COUNT; bindingId++)
						addBuffer(m_descriptor->VertexBuffer(bindingId)->Buffer());

					// Instance buffers:
					for (size_t bindingId = 0; bindingId < INSTANCE_BUFFER_COUNT; bindingId++)
						addBuffer(m_descriptor->InstanceBuffer(bindingId)->Buffer());

					// Bind buffers
					vkCmdBindVertexBuffers(*commandBuffer, 0, VERTEX_BINDING_COUNT, vertexBindings.data(), vertexBindingOffsets.data());
				}
				
				// Execute the pipeline:
				{
					vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

					UpdateDescriptors(bufferInfo);
					BindDescriptors(bufferInfo, VK_PIPELINE_BIND_POINT_GRAPHICS);

					if (VERTEX_BINDING_COUNT > 0)
						vkCmdBindVertexBuffers(*commandBuffer, 0, VERTEX_BINDING_COUNT, vertexBindings.data(), vertexBindingOffsets.data());

					vkCmdBindIndexBuffer(*commandBuffer, *m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
					vkCmdDrawIndexed(*commandBuffer, INDEX_COUNT, INSTANCE_COUNT, 0, 0, 0);
				}
				commandBuffer->RecordBufferDependency(this);
			}
		}
	}
}
#pragma warning(default: 26812)

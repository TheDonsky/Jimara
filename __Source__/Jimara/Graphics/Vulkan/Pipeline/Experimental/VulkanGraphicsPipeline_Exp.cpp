#include "VulkanGraphicsPipeline_Exp.h"
#include "../VulkanShader.h"
#include "../../../../Math/Helpers.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			namespace {
				struct VulkanGraphicsPipeline_Identifier {
					Reference<const VulkanRenderPass> renderPass;
					Reference<const SPIRV_Binary> vertexShader;
					Reference<const SPIRV_Binary> fragmentShader;
					Graphics::Experimental::GraphicsPipeline::BlendMode blendMode = 
						Graphics::Experimental::GraphicsPipeline::BlendMode::REPLACE;
					Graphics::Experimental::GraphicsPipeline::IndexType indexType = 
						Graphics::Experimental::GraphicsPipeline::IndexType::TRIANGLE;

					struct LayoutEntry {
						uint32_t bufferId = 0u;
						uint32_t location = 0u;
						uint32_t bufferElementSize = 0u;
						uint32_t bufferElementOffset = 0u;
						Graphics::Experimental::GraphicsPipeline::VertexInputInfo::InputRate inputRate =
							Graphics::Experimental::GraphicsPipeline::VertexInputInfo::InputRate::VERTEX;
						Graphics::Experimental::GraphicsPipeline::VertexInputInfo::AttributeType attributeType =
							Graphics::Experimental::GraphicsPipeline::VertexInputInfo::AttributeType::TYPE_COUNT;
					};
					Stacktor<LayoutEntry, 4u> layoutEntries;

					inline bool operator==(const VulkanGraphicsPipeline_Identifier& other)const {
						if (renderPass != other.renderPass) return false;
						else if (vertexShader != other.vertexShader) return false;
						else if (fragmentShader != other.fragmentShader) return false;
						else if (blendMode != other.blendMode) return false;
						else if (indexType != other.indexType) return false;
						else if (layoutEntries.Size() != other.layoutEntries.Size()) return false;
						else for (size_t i = 0u; i < layoutEntries.Size(); i++) {
							const LayoutEntry& a = layoutEntries[i];
							const LayoutEntry& b = other.layoutEntries[i];
							if (a.bufferId != b.bufferId) return false;
							else if (a.location != b.location) return false;
							else if (a.bufferElementSize != b.bufferElementSize) return false;
							else if (a.bufferElementOffset != b.bufferElementOffset) return false;
							else if (a.inputRate != b.inputRate) return false;
							else if (a.attributeType != b.attributeType) return false;
						}
						return true;
					}
					inline bool operator!=(const VulkanGraphicsPipeline_Identifier& other)const {
						return !((*this) == other);
					}
				};
			}
		}
	}
}

namespace std {
	template<>
	struct hash<Jimara::Graphics::Vulkan::VulkanGraphicsPipeline_Identifier> {
		size_t operator()(const Jimara::Graphics::Vulkan::VulkanGraphicsPipeline_Identifier& key)const {
			static_assert(std::is_same_v<
				std::remove_pointer_t<decltype(&key)>, 
				const Jimara::Graphics::Vulkan::VulkanGraphicsPipeline_Identifier>);
			auto hashOf = [](auto v) { 
				return std::hash<decltype(v)>()(v); 
			};
			
			size_t cumulativeHash = hashOf(key.renderPass.operator->());
			auto includeHash = [&](auto v) { 
				cumulativeHash = Jimara::MergeHashes(cumulativeHash, hashOf(v));
			};

			includeHash(key.vertexShader.operator->());
			includeHash(key.fragmentShader.operator->());
			includeHash(key.blendMode);
			includeHash(key.indexType);

			for (size_t i = 0u; i < key.layoutEntries.Size(); i++) {
				const Jimara::Graphics::Vulkan::VulkanGraphicsPipeline_Identifier::LayoutEntry& entry = key.layoutEntries[i];
				includeHash(entry.bufferId);
				includeHash(entry.location);
				includeHash(entry.bufferElementSize);
				includeHash(entry.bufferElementOffset);
				includeHash(entry.inputRate);
				includeHash(entry.attributeType);
			}

			return cumulativeHash;
		}
	};
}

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
		namespace Experimental {
			struct VulkanGraphicsPipeline::Helpers {
				inline static VulkanGraphicsPipeline_Identifier CreatePipelineIdentifier(
					VulkanRenderPass* renderPass, const Descriptor& pipelineDescriptor) {
					if (renderPass == nullptr) return VulkanGraphicsPipeline_Identifier{};
					
					// Thread local buffers for calculations and fail state: 
					struct KnownLayoutEntry {
						uint32_t location = 0u;
						VertexBuffer::AttributeInfo::Type format = VertexBuffer::AttributeInfo::Type::TYPE_COUNT;
						size_t firstNameAliasId = ~size_t(0u);
						size_t lastAliasId = ~size_t(0u);
					};

					struct KnownAlias {
						std::string_view alias;
						size_t nextNameAliasId = ~size_t(0u);
					};

					static thread_local Stacktor<KnownLayoutEntry> knownEntries;
					static thread_local Stacktor<KnownAlias> knownAliases;

					auto cleanThreadLocalStorage = [&]() {
						knownEntries.Clear();
						knownAliases.Clear();
					};

					auto fail = [&](const auto&... message) {
						renderPass->Device()->Log()->Error("VulkanGraphicsPipeline::Helpers::CreatePipelineIdentifier - ", message...);
						cleanThreadLocalStorage();
						return VulkanGraphicsPipeline_Identifier{};
					};

					// Verify shaders:
					{
						if (pipelineDescriptor.vertexShader == nullptr)
							return fail("Vertex shader not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

						if ((pipelineDescriptor.vertexShader->ShaderStages() & StageMask(PipelineStage::VERTEX)) == 0u)
							return fail(
								"pipelineDescriptor.vertexShader expected to be compatible with PipelineStage::VERTEX, but it is not! ",
								"[File: ", __FILE__, "; Line: ", __LINE__, "]");

						if (pipelineDescriptor.fragmentShader == nullptr)
							return fail("Fragment shader not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

						if ((pipelineDescriptor.fragmentShader->ShaderStages() & StageMask(PipelineStage::FRAGMENT)) == 0u)
							return fail(
								"pipelineDescriptor.fragmentShader expected to be compatible with PipelineStage::FRAGMENT, but it is not! ",
								"[File: ", __FILE__, "; Line: ", __LINE__, "]");
					}

					// Compress vertex shader input down to knownEntries and knownAliases:
					cleanThreadLocalStorage();
					for (size_t inputId = 0u; inputId < pipelineDescriptor.vertexShader->ShaderInputCount(); inputId++) {
						const SPIRV_Binary::ShaderInputInfo& inputInfo = pipelineDescriptor.vertexShader->ShaderInput(inputId);
						
						// Check existing entries:
						bool isNewAlias = false;
						for (size_t i = 0u; i < knownEntries.Size(); i++) {
							KnownLayoutEntry& entry = knownEntries[i];
							if (entry.location != inputInfo.location) continue;
							if (entry.format >= VertexBuffer::AttributeInfo::Type::TYPE_COUNT)
								entry.format = inputInfo.format;
							else if (inputInfo.format < VertexBuffer::AttributeInfo::Type::TYPE_COUNT && inputInfo.format != entry.format)
								return fail(
									"More than one attribute type detected on the same location slot(", inputInfo.location, ")! ",
									"[File: ", __FILE__, "; Line: ", __LINE__, "]");
							
							knownAliases[entry.lastAliasId].nextNameAliasId = knownAliases.Size();
							entry.lastAliasId = knownAliases.Size();
							
							KnownAlias alias = {};
							alias.alias = inputInfo.name;
							alias.nextNameAliasId = ~size_t(0u);
							knownAliases.Push(alias);
							
							isNewAlias = true;
							break;
						}
						if (isNewAlias) continue;
						
						// Create new entry:
						{
							KnownLayoutEntry entry = {};
							entry.location = inputInfo.location;
							entry.format = inputInfo.format;
							entry.firstNameAliasId = knownAliases.Size();
							entry.lastAliasId = entry.firstNameAliasId;
							knownEntries.Push(entry);

							KnownAlias alias = {};
							alias.alias = inputInfo.name;
							alias.nextNameAliasId = ~size_t(0u);
							knownAliases.Push(alias);
						}
					}

					// Define basic parameters for the result:
					VulkanGraphicsPipeline_Identifier pipelineId = {};
					pipelineId.renderPass = renderPass;
					pipelineId.vertexShader = pipelineDescriptor.vertexShader;
					pipelineId.fragmentShader = pipelineDescriptor.fragmentShader;
					pipelineId.blendMode = pipelineDescriptor.blendMode;
					pipelineId.indexType = pipelineDescriptor.indexType;

					// Map known entries to descriptor input:
					for (size_t entryId = 0u; entryId < knownEntries.Size(); entryId++) {
						const KnownLayoutEntry& inputInfo = knownEntries[entryId];

						// Make sure we catch unsupported format before it's too late:
						if (inputInfo.format >= VertexBuffer::AttributeInfo::Type::TYPE_COUNT)
							return fail(
								"Vertex input at location ", inputInfo.location, " has unsupported type! ",
								"[File: ", __FILE__, "; Line: ", __LINE__, "]");

						// Try finding:
						auto tryFind = [&](const auto& matchFn) {
							for (size_t bufferId = 0u; bufferId < pipelineDescriptor.vertexInput.Size(); bufferId++) {
								const GraphicsPipeline::VertexInputInfo& bufferLayout = pipelineDescriptor.vertexInput[bufferId];
								for (size_t bufferEntryId = 0u; bufferEntryId < bufferLayout.locations.Size(); bufferEntryId++) {
									const GraphicsPipeline::VertexInputInfo::LocationInfo& locationInfo = bufferLayout.locations[bufferEntryId];
									if (matchFn(locationInfo)) continue;
									
									VulkanGraphicsPipeline_Identifier::LayoutEntry entry = {};
									entry.bufferId = static_cast<uint32_t>(bufferId);
									entry.location = inputInfo.location;
									entry.bufferElementSize = static_cast<uint32_t>(bufferLayout.bufferElementSize);
									entry.bufferElementOffset = static_cast<uint32_t>(locationInfo.bufferElementOffset);
									entry.inputRate = bufferLayout.inputRate;
									entry.attributeType = inputInfo.format;
									pipelineId.layoutEntries.Push(entry);
									
									return true;
								}
							}
							return false;
						};
						if (!tryFind([&](const auto& info) { return info.location.has_value() && info.location.value() == inputInfo.location; })) {
							bool found = false;
							size_t aliasIndex = inputInfo.firstNameAliasId;
							while (aliasIndex < knownAliases.Size()) {
								const KnownAlias& alias = knownAliases[aliasIndex];
								aliasIndex = alias.nextNameAliasId;
								if (tryFind([&](const auto& info) { return info.name == alias.alias; })) {
									found = true;
									break;
								}
							}
							if (!found)
								return fail(
									"Failed to find vertex input for location ", inputInfo.location, "! ",
									"[File:", __FILE__, "; Line: ", __LINE__, "]");
						}
						if (pipelineId.layoutEntries.Size() != (entryId + 1u))
							return fail(
								"Internal error (pipelineId.layoutEntries.Size() != (entryId + 1u))! ",
								"[File:", __FILE__, "; Line: ", __LINE__, "]");
					}

					cleanThreadLocalStorage();
					return pipelineId;
				}



				inline static VkPipeline CreateVulkanPipeline(
					VulkanShader* vertexShader, VulkanShader* fragmentShader,
					const VulkanGraphicsPipeline_Identifier& pipelineShape, VkPipelineLayout pipelineLayout) {
					auto fail = [&](const auto&... message) {
						pipelineShape.renderPass->Device()->Log()->Error("VulkanGraphicsPipeline::Helpers::CreateVulkanPipeline - ", message...);
						return VK_NULL_HANDLE;
					};


					// ShaderStageInfos:
					VkPipelineShaderStageCreateInfo shaderStages[2] = { {}, {} };

					if (vertexShader != nullptr) {
						VkPipelineShaderStageCreateInfo& vertShaderStageInfo = shaderStages[0];
						vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
						vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
						vertShaderStageInfo.module = *vertexShader;
						vertShaderStageInfo.pName = "main";
					}
					else return fail("Vertex shader module could not be created! [File:", __FILE__, "; Line: ", __LINE__, "]");

					if (fragmentShader != nullptr) {
						VkPipelineShaderStageCreateInfo& fragShaderStageInfo = shaderStages[1];
						fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
						fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
						fragShaderStageInfo.module = *fragmentShader;
						fragShaderStageInfo.pName = "main";
					}
					else return fail("Fragment shader module could not be creeated! [File:", __FILE__, "; Line: ", __LINE__, "]");


					// Vertex input:
					VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
					{
						static thread_local std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions;
						static thread_local std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
						vertexInputBindingDescriptions.clear();
						vertexInputAttributeDescriptions.clear();

						for (size_t layoutEntryId = 0u; layoutEntryId < pipelineShape.layoutEntries.Size(); layoutEntryId++) {
							const VulkanGraphicsPipeline_Identifier::LayoutEntry& layoutEntry = pipelineShape.layoutEntries[layoutEntryId];

							// Update VkVertexInputBindingDescription:
							{
								while (layoutEntry.bufferId >= vertexInputBindingDescriptions.size())
									vertexInputBindingDescriptions.push_back({});
								VkVertexInputBindingDescription& desc = vertexInputBindingDescriptions[layoutEntry.bufferId];
								desc.binding = layoutEntry.bufferId;
								desc.stride = layoutEntry.bufferElementSize;
								desc.inputRate = (layoutEntry.inputRate == VertexInputInfo::InputRate::VERTEX)
									? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
							}

							// Add VkVertexInputAttributeDescription:
							{
								if (layoutEntry.attributeType >= VertexBuffer::AttributeInfo::Type::TYPE_COUNT)
									return fail(
										"Unsupported attribute type for location ", layoutEntry.location, "! ",
										"[File:", __FILE__, "; Line: ", __LINE__, "]");

								struct VkAttributeType {
									VkFormat format = VK_FORMAT_MAX_ENUM;
									uint32_t bindingCount = 0u;
									uint32_t offsetDelta = 0u;
								};

								static const VkAttributeType* BINDING_TYPE_TO_FORMAT = []() -> VkAttributeType* {
									const uint8_t BINDING_TYPE_COUNT = static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::TYPE_COUNT);
									static VkAttributeType bindingTypeToFormats[BINDING_TYPE_COUNT];

									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::FLOAT)] = { 
										VK_FORMAT_R32_SFLOAT, 1u, static_cast<uint32_t>(sizeof(float)) };
									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::FLOAT2)] = { 
										VK_FORMAT_R32G32_SFLOAT, 1u, static_cast<uint32_t>(sizeof(Vector2)) };
									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::FLOAT3)] = { 
										VK_FORMAT_R32G32B32_SFLOAT, 1u, static_cast<uint32_t>(sizeof(Vector3)) };
									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::FLOAT4)] = { 
										VK_FORMAT_R32G32B32A32_SFLOAT, 1u, static_cast<uint32_t>(sizeof(Vector4)) };

									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::INT)] = {
										VK_FORMAT_R32_SINT, 1u, static_cast<uint32_t>(sizeof(int)) };
									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::INT2)] = {
										VK_FORMAT_R32G32_SINT, 1u, static_cast<uint32_t>(sizeof(Int2)) };
									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::INT3)] = {
										VK_FORMAT_R32G32B32_SINT, 1u, static_cast<uint32_t>(sizeof(Int3)) };
									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::INT4)] = {
										VK_FORMAT_R32G32B32A32_SINT, 1u, static_cast<uint32_t>(sizeof(Int4)) };

									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::UINT)] = {
										VK_FORMAT_R32_UINT, 1u, static_cast<uint32_t>(sizeof(uint32_t)) };
									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::UINT2)] = {
										VK_FORMAT_R32G32_UINT, 1u, static_cast<uint32_t>(sizeof(Size2)) };
									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::UINT3)] = {
										VK_FORMAT_R32G32B32_UINT, 1u, static_cast<uint32_t>(sizeof(Size3)) };
									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::UINT4)] = {
										VK_FORMAT_R32G32B32A32_UINT, 1u, static_cast<uint32_t>(sizeof(Size4)) };

									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::BOOL32)] = {
										VK_FORMAT_R32_UINT, 1u, static_cast<uint32_t>(sizeof(VkBool32)) };

									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::MAT_2X2)] = {
										VK_FORMAT_R32G32_SFLOAT, 2u, static_cast<uint32_t>([]() {
											Matrix2 mat = {}; return ((char*)(&mat[1])) - ((char*)(&mat));
											}()) };
									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::MAT_3X3)] = {
										VK_FORMAT_R32G32B32_SFLOAT, 3u, static_cast<uint32_t>([]() {
											Matrix3 mat = {}; return ((char*)(&mat[1])) - ((char*)(&mat));
											}()) };
									bindingTypeToFormats[static_cast<uint8_t>(VertexBuffer::AttributeInfo::Type::MAT_4X4)] = {
										VK_FORMAT_R32G32B32A32_SFLOAT, 4u, static_cast<uint32_t>([]() {
											Matrix4 mat = {}; return ((char*)(&mat[1])) - ((char*)(&mat));
											}()) };

									return bindingTypeToFormats;
								}();
								const VkAttributeType attributeTypeInfo = BINDING_TYPE_TO_FORMAT[static_cast<size_t>(layoutEntry.attributeType)];

								VkVertexInputAttributeDescription attributeDescription = {};
								attributeDescription.location = layoutEntry.location;
								attributeDescription.binding = layoutEntry.bufferId;
								attributeDescription.format = attributeTypeInfo.format;
								attributeDescription.offset = layoutEntry.bufferElementOffset;

								for (size_t i = 0u; i < attributeTypeInfo.bindingCount; i++) {
									vertexInputAttributeDescriptions.push_back(attributeDescription);
									attributeDescription.offset += attributeTypeInfo.offsetDelta;
									attributeDescription.location++;
								}
							}
						}

						// Define vertex input:
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
						inputAssembly.topology =
							(pipelineShape.indexType == GraphicsPipeline::IndexType::TRIANGLE) ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST : // Triangle from every 3 vertices without reuse...
							(pipelineShape.indexType == GraphicsPipeline::IndexType::EDGE) ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST : // Edges from every 2 vertices without reuse...
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


					// Rasterizer:
					VkPipelineRasterizationStateCreateInfo rasterizer = {};
					{
						// __TODO__: Investigate further...

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
						multisampling.rasterizationSamples = dynamic_cast<VulkanDevice*>(pipelineShape.renderPass->Device())->
							PhysicalDeviceInfo()->SampleCountFlags(pipelineShape.renderPass->SampleCount());
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
						// __TODO__: Based on blend mode, set different values

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
					while (colorBlendAttachments.size() < pipelineShape.renderPass->ColorAttachmentCount())
						colorBlendAttachments.push_back(colorBlendAttachment);

					VkPipelineColorBlendStateCreateInfo colorBlending = {};
					{
						colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
						colorBlending.logicOpEnable = VK_FALSE;
						colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
						colorBlending.attachmentCount = static_cast<uint32_t>(pipelineShape.renderPass->ColorAttachmentCount());
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
						pipelineInfo.pDepthStencilState = pipelineShape.renderPass->HasDepthAttachment() ? &depthStencil : nullptr; // This tells to check depth...
						pipelineInfo.pColorBlendState = &colorBlending; // Forgot already... Has something to do with how we treat overlapping fragment colors.
						pipelineInfo.pDynamicState = &dynamicState; // Defines, what aspects of our pipeline can change during runtime.
						pipelineInfo.layout = pipelineLayout; // Pretty sure this is important as hell.
						pipelineInfo.renderPass = *pipelineShape.renderPass; // Pretty sure this is important as hell.
						pipelineInfo.subpass = 0; // Index of the subpass, out pipeline will be used at (Yep, the render pass so far consists of a single pass)
						pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
						pipelineInfo.basePipelineIndex = -1; // Optional
					}

					VkPipeline graphicsPipeline = VK_NULL_HANDLE;
					if (vkCreateGraphicsPipelines(*dynamic_cast<VulkanDevice*>(pipelineShape.renderPass->Device())
						, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
						return fail("VulkanGraphicsPipeline - Failed to create graphics pipeline!");
					else return graphicsPipeline;
				}
			};

			Reference<VulkanGraphicsPipeline> VulkanGraphicsPipeline::Get(VulkanRenderPass* renderPass, const Descriptor& pipelineDescriptor) {
				VulkanGraphicsPipeline_Identifier identifier = Helpers::CreatePipelineIdentifier(renderPass, pipelineDescriptor);
				if (identifier.renderPass == nullptr) return nullptr;

				// __TODO__: Implement this crap!
				renderPass->Device()->Log()->Error("VulkanGraphicsPipeline::Get - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return nullptr;
			}

			VulkanGraphicsPipeline::VulkanGraphicsPipeline() {
				// __TODO__: Implement this crap!
			}

			VulkanGraphicsPipeline::~VulkanGraphicsPipeline() {
				// __TODO__: Implement this crap!
			}

			Reference<VertexInput> VulkanGraphicsPipeline::CreateVertexInput(
				const ResourceBinding<Graphics::ArrayBuffer>** vertexBuffers,
				const ResourceBinding<Graphics::ArrayBuffer>* indexBuffer) {
				// __TODO__: Implement this crap!
				Device()->Log()->Error("VulkanGraphicsPipeline::CreateVertexInput - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return nullptr;
			}

			void VulkanGraphicsPipeline::Draw(CommandBuffer* commandBuffer, size_t indexCount, size_t instanceCount) {
				// __TODO__: Implement this crap!
			}

			void VulkanGraphicsPipeline::DrawIndirect(CommandBuffer* commandBuffer, IndirectDrawBuffer* indirectBuffer, size_t drawCount) {
				// __TODO__: Implement this crap!
			}
		}
		}
	}
}

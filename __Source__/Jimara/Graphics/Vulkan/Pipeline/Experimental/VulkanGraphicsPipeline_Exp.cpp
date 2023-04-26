#include "VulkanGraphicsPipeline_Exp.h"
#include "../../../../Math/Helpers.h"
#include "../../Memory/Buffers/VulkanIndirectBuffers.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			namespace {
				struct VulkanGraphicsPipeline_Identifier {
					Reference<const VulkanRenderPass> renderPass;
					Reference<const SPIRV_Binary> vertexShader;
					Reference<const SPIRV_Binary> fragmentShader;
					uint32_t vertexBufferCount = 0u;
					Graphics::GraphicsPipeline::BlendMode blendMode = Graphics::GraphicsPipeline::BlendMode::REPLACE;
					Graphics::GraphicsPipeline::IndexType indexType = Graphics::GraphicsPipeline::IndexType::TRIANGLE;

					struct LayoutEntry {
						uint32_t bufferId = 0u;
						uint32_t location = 0u;
						uint32_t bufferElementSize = 0u;
						uint32_t bufferElementOffset = 0u;
						Graphics::GraphicsPipeline::VertexInputInfo::InputRate inputRate =
							Graphics::GraphicsPipeline::VertexInputInfo::InputRate::VERTEX;
						SPIRV_Binary::ShaderInputInfo::Type attributeType =
							SPIRV_Binary::ShaderInputInfo::Type::TYPE_COUNT;
					};
					Stacktor<LayoutEntry, 4u> layoutEntries;

					inline bool operator==(const VulkanGraphicsPipeline_Identifier& other)const {
						if (renderPass != other.renderPass) return false;
						else if (vertexShader != other.vertexShader) return false;
						else if (fragmentShader != other.fragmentShader) return false;
						else if (vertexBufferCount != other.vertexBufferCount) return false;
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
			includeHash(key.vertexBufferCount);
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
						SPIRV_Binary::ShaderInputInfo::Type format = SPIRV_Binary::ShaderInputInfo::Type::TYPE_COUNT;
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

						if ((pipelineDescriptor.vertexShader->ShaderStages() & PipelineStage::VERTEX) == PipelineStage::NONE)
							return fail(
								"pipelineDescriptor.vertexShader expected to be compatible with PipelineStage::VERTEX, but it is not! ",
								"[File: ", __FILE__, "; Line: ", __LINE__, "]");

						if (pipelineDescriptor.fragmentShader == nullptr)
							return fail("Fragment shader not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

						if ((pipelineDescriptor.fragmentShader->ShaderStages() & PipelineStage::FRAGMENT) == PipelineStage::NONE)
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
							if (entry.format >= SPIRV_Binary::ShaderInputInfo::Type::TYPE_COUNT)
								entry.format = inputInfo.format;
							else if (inputInfo.format < SPIRV_Binary::ShaderInputInfo::Type::TYPE_COUNT && inputInfo.format != entry.format)
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
					pipelineId.vertexBufferCount = static_cast<uint32_t>(pipelineDescriptor.vertexInput.Size());
					pipelineId.blendMode = pipelineDescriptor.blendMode;
					pipelineId.indexType = pipelineDescriptor.indexType;

					// Map known entries to descriptor input:
					for (size_t entryId = 0u; entryId < knownEntries.Size(); entryId++) {
						const KnownLayoutEntry& inputInfo = knownEntries[entryId];

						// Make sure we catch unsupported format before it's too late:
						if (inputInfo.format >= SPIRV_Binary::ShaderInputInfo::Type::TYPE_COUNT)
							return fail(
								"Vertex input at location ", inputInfo.location, " has unsupported type! ",
								"[File: ", __FILE__, "; Line: ", __LINE__, "]");

						// Try finding:
						auto tryFind = [&](const auto& matchFn) {
							for (size_t bufferId = 0u; bufferId < pipelineDescriptor.vertexInput.Size(); bufferId++) {
								const GraphicsPipeline::VertexInputInfo& bufferLayout = pipelineDescriptor.vertexInput[bufferId];
								for (size_t bufferEntryId = 0u; bufferEntryId < bufferLayout.locations.Size(); bufferEntryId++) {
									const GraphicsPipeline::VertexInputInfo::LocationInfo& locationInfo = bufferLayout.locations[bufferEntryId];
									if (!matchFn(locationInfo)) continue;
									
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
								while (layoutEntry.bufferId >= vertexInputBindingDescriptions.size()) {
									VkVertexInputBindingDescription desc = {};
									desc.binding = static_cast<uint32_t>(vertexInputBindingDescriptions.size());
									desc.stride = 4u;
									desc.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
									vertexInputBindingDescriptions.push_back(desc);
								}
								VkVertexInputBindingDescription& desc = vertexInputBindingDescriptions[layoutEntry.bufferId];
								desc.stride = layoutEntry.bufferElementSize;
								desc.inputRate = (layoutEntry.inputRate == VertexInputInfo::InputRate::VERTEX)
									? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
							}

							// Add VkVertexInputAttributeDescription:
							{
								if (layoutEntry.attributeType >= SPIRV_Binary::ShaderInputInfo::Type::TYPE_COUNT)
									return fail(
										"Unsupported attribute type for location ", layoutEntry.location, "! ",
										"[File:", __FILE__, "; Line: ", __LINE__, "]");

								struct VkAttributeType {
									VkFormat format = VK_FORMAT_MAX_ENUM;
									uint32_t bindingCount = 0u;
									uint32_t offsetDelta = 0u;
								};

								static const VkAttributeType* BINDING_TYPE_TO_FORMAT = []() -> VkAttributeType* {
									const uint8_t BINDING_TYPE_COUNT = static_cast<uint8_t>(SPIRV_Binary::ShaderInputInfo::Type::TYPE_COUNT);
									static VkAttributeType bindingTypeToFormats[BINDING_TYPE_COUNT];

									bindingTypeToFormats[static_cast<uint8_t>(SPIRV_Binary::ShaderInputInfo::Type::FLOAT)] = {
										VK_FORMAT_R32_SFLOAT, 1u, static_cast<uint32_t>(sizeof(float)) };
									bindingTypeToFormats[static_cast<uint8_t>(SPIRV_Binary::ShaderInputInfo::Type::FLOAT2)] = {
										VK_FORMAT_R32G32_SFLOAT, 1u, static_cast<uint32_t>(sizeof(Vector2)) };
									bindingTypeToFormats[static_cast<uint8_t>(SPIRV_Binary::ShaderInputInfo::Type::FLOAT3)] = {
										VK_FORMAT_R32G32B32_SFLOAT, 1u, static_cast<uint32_t>(sizeof(Vector3)) };
									bindingTypeToFormats[static_cast<uint8_t>(SPIRV_Binary::ShaderInputInfo::Type::FLOAT4)] = {
										VK_FORMAT_R32G32B32A32_SFLOAT, 1u, static_cast<uint32_t>(sizeof(Vector4)) };

									bindingTypeToFormats[static_cast<uint8_t>(SPIRV_Binary::ShaderInputInfo::Type::INT)] = {
										VK_FORMAT_R32_SINT, 1u, static_cast<uint32_t>(sizeof(int)) };
									bindingTypeToFormats[static_cast<uint8_t>(SPIRV_Binary::ShaderInputInfo::Type::INT2)] = {
										VK_FORMAT_R32G32_SINT, 1u, static_cast<uint32_t>(sizeof(Int2)) };
									bindingTypeToFormats[static_cast<uint8_t>(SPIRV_Binary::ShaderInputInfo::Type::INT3)] = {
										VK_FORMAT_R32G32B32_SINT, 1u, static_cast<uint32_t>(sizeof(Int3)) };
									bindingTypeToFormats[static_cast<uint8_t>(SPIRV_Binary::ShaderInputInfo::Type::INT4)] = {
										VK_FORMAT_R32G32B32A32_SINT, 1u, static_cast<uint32_t>(sizeof(Int4)) };

									bindingTypeToFormats[static_cast<uint8_t>(SPIRV_Binary::ShaderInputInfo::Type::UINT)] = {
										VK_FORMAT_R32_UINT, 1u, static_cast<uint32_t>(sizeof(uint32_t)) };
									bindingTypeToFormats[static_cast<uint8_t>(SPIRV_Binary::ShaderInputInfo::Type::UINT2)] = {
										VK_FORMAT_R32G32_UINT, 1u, static_cast<uint32_t>(sizeof(Size2)) };
									bindingTypeToFormats[static_cast<uint8_t>(SPIRV_Binary::ShaderInputInfo::Type::UINT3)] = {
										VK_FORMAT_R32G32B32_UINT, 1u, static_cast<uint32_t>(sizeof(Size3)) };
									bindingTypeToFormats[static_cast<uint8_t>(SPIRV_Binary::ShaderInputInfo::Type::UINT4)] = {
										VK_FORMAT_R32G32B32A32_UINT, 1u, static_cast<uint32_t>(sizeof(Size4)) };

									bindingTypeToFormats[static_cast<uint8_t>(SPIRV_Binary::ShaderInputInfo::Type::BOOL32)] = {
										VK_FORMAT_R32_UINT, 1u, static_cast<uint32_t>(sizeof(VkBool32)) };

									bindingTypeToFormats[static_cast<uint8_t>(SPIRV_Binary::ShaderInputInfo::Type::MAT_2X2)] = {
										VK_FORMAT_R32G32_SFLOAT, 2u, static_cast<uint32_t>([]() {
											Matrix2 mat = {}; return ((char*)(&mat[1])) - ((char*)(&mat));
											}()) };
									bindingTypeToFormats[static_cast<uint8_t>(SPIRV_Binary::ShaderInputInfo::Type::MAT_3X3)] = {
										VK_FORMAT_R32G32B32_SFLOAT, 3u, static_cast<uint32_t>([]() {
											Matrix3 mat = {}; return ((char*)(&mat[1])) - ((char*)(&mat));
											}()) };
									bindingTypeToFormats[static_cast<uint8_t>(SPIRV_Binary::ShaderInputInfo::Type::MAT_4X4)] = {
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
						colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
						colorBlendAttachment.blendEnable = (pipelineShape.blendMode == BlendMode::REPLACE) ? VK_FALSE : VK_TRUE;
						if (pipelineShape.blendMode == BlendMode::ALPHA_BLEND) {
							// Color = A.rgb * (1 - B.a) + B.rgb * B.a
							colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
							colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
							colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

							// Alpha = 1 - (1 - A.a) * (1 - B.a) = 1 - (1 - A.a - B.a + A.a * B.a) = A.a + B.a - A.a * B.a = A.a * (1 - B.a) + B.a * 1
							colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
							colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
							colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
						}
						else if (pipelineShape.blendMode == BlendMode::ADDITIVE) {
							// Color = A.rgb + B.rgb * B.a
							colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
							colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
							colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

							// Arbitrarily, the same as ALPHA_BLEND case
							colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
							colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
							colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
						}
						else {
							// Color = B.rgb = A.rgb * 0 + B.rgb * 1
							colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
							colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
							colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

							// Alpha = B.a = A.a * 0 + B.a * 1
							colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
							colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
							colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
						}
					}
					static thread_local std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
					colorBlendAttachments.clear();
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
						return fail("Failed to create graphics pipeline!");
					else return graphicsPipeline;
				}

#pragma warning(disable: 4250)
				class CachedPipeline
					: public virtual VulkanGraphicsPipeline
					, public virtual ObjectCache<VulkanGraphicsPipeline_Identifier>::StoredObject {
				private:
					inline CachedPipeline(
						VulkanPipeline::BindingSetBuilder&& builder, 
						VkPipeline pipeline, size_t vertexBufferCount,
						VulkanShader* vertexShader, VulkanShader* fragmentShader)
						: VulkanPipeline(std::move(builder))
						, VulkanGraphicsPipeline(pipeline, vertexBufferCount, vertexShader, fragmentShader) {};

				public:
					inline virtual ~CachedPipeline() {}

					inline static Reference<CachedPipeline> Create(const VulkanGraphicsPipeline_Identifier& identifier) {
						VulkanDevice* device = dynamic_cast<VulkanDevice*>(identifier.renderPass->Device());
						auto fail = [&](const auto&... message) {
							device->Log()->Error("VulkanGraphicsPipeline::Helpers::CachedPipeline::Create - ", message...);
							return nullptr;
						};
						
						VulkanPipeline::BindingSetBuilder builder(device);
						if (!builder.IncludeShaderBindings(identifier.vertexShader))
							return fail("Could not configure binding set shape for the vertex shader! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						if (!builder.IncludeShaderBindings(identifier.fragmentShader))
							return fail("Could not configure binding set shape for the fragment shader! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						if (!builder.Finish())
							return fail("Could create pipeline layout! [File: ", __FILE__, "; Line: ", __LINE__, "]");

						const Reference<VulkanShader> vertexShader = Object::Instantiate<VulkanShader>(device, identifier.vertexShader);
						const Reference<VulkanShader> fragmentShader = Object::Instantiate<VulkanShader>(device, identifier.fragmentShader);

						VkPipeline pipeline = CreateVulkanPipeline(vertexShader, fragmentShader, identifier, builder.PipelineLayout());
						if (pipeline == VK_NULL_HANDLE) return nullptr;

						const Reference<CachedPipeline> result = new CachedPipeline(
							std::move(builder), pipeline, identifier.vertexBufferCount, vertexShader, fragmentShader);
						result->ReleaseRef();
						return result;
					}
				};
#pragma warning(default: 4250)

				class PipelineCache : public virtual ObjectCache<VulkanGraphicsPipeline_Identifier> {
				public:
					inline virtual ~PipelineCache() {}

					static Reference<VulkanGraphicsPipeline> GetFor(const VulkanGraphicsPipeline_Identifier& identifier) {
						static PipelineCache cache;
						return cache.GetCachedOrCreate(identifier, false, [&]() { return CachedPipeline::Create(identifier); });
					}
				};
			};

			Reference<VulkanGraphicsPipeline> VulkanGraphicsPipeline::Get(VulkanRenderPass* renderPass, const Descriptor& pipelineDescriptor) {
				VulkanGraphicsPipeline_Identifier identifier = Helpers::CreatePipelineIdentifier(renderPass, pipelineDescriptor);
				if (identifier.renderPass == nullptr) return nullptr;
				else return Helpers::PipelineCache::GetFor(identifier);
			}

			VulkanGraphicsPipeline::VulkanGraphicsPipeline(
				VkPipeline pipeline, size_t vertexBufferCount,
				VulkanShader* vertexShader, VulkanShader* fragmentShader)
				: m_pipeline(pipeline), m_vertexBufferCount(vertexBufferCount)
				, m_vertexShader(vertexShader), m_fragmentShader(fragmentShader) {
				assert(m_pipeline != VK_NULL_HANDLE);
			}

			VulkanGraphicsPipeline::~VulkanGraphicsPipeline() {
				if (m_pipeline != VK_NULL_HANDLE)
					vkDestroyPipeline(*Device(), m_pipeline, nullptr);
			}

			Reference<VertexInput> VulkanGraphicsPipeline::CreateVertexInput(
				const ResourceBinding<Graphics::ArrayBuffer>* const* vertexBuffers,
				const ResourceBinding<Graphics::ArrayBuffer>* indexBuffer) {
				auto fail = [&](const auto... message) {
					Device()->Log()->Error("VulkanGraphicsPipeline::CreateVertexInput - ", message...);
					return nullptr;
				};

				for (size_t i = 0u; i < m_vertexBufferCount; i++)
					if (vertexBuffers[i] == nullptr)
						return fail("vertexBuffers array contains null entries! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				return Object::Instantiate<VulkanVertexInput>(Device(), vertexBuffers, m_vertexBufferCount, indexBuffer);
			}

			void VulkanGraphicsPipeline::Draw(CommandBuffer* commandBuffer, size_t indexCount, size_t instanceCount) {
				VulkanCommandBuffer* commands = dynamic_cast<VulkanCommandBuffer*>(commandBuffer);
				if (commands == nullptr) {
					Device()->Log()->Error(
						"VulkanGraphicsPipeline::Draw - Invalid command buffer provided! ",
						"[File: ", __FILE__, "; Line: ", __LINE__, "]");
					return;
				}
				
				vkCmdBindPipeline(*commands, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
				vkCmdDrawIndexed(*commands, static_cast<uint32_t>(indexCount), static_cast<uint32_t>(instanceCount), 0u, 0u, 0u);
				commands->RecordBufferDependency(this);
			}

			void VulkanGraphicsPipeline::DrawIndirect(CommandBuffer* commandBuffer, IndirectDrawBuffer* indirectBuffer, size_t drawCount) {
				VulkanCommandBuffer* commands = dynamic_cast<VulkanCommandBuffer*>(commandBuffer);
				if (commands == nullptr) {
					Device()->Log()->Error(
						"VulkanGraphicsPipeline::DrawIndirect - Invalid command buffer provided! ",
						"[File: ", __FILE__, "; Line: ", __LINE__, "]");
					return;
				}
				
				VulkanArrayBuffer* vulkanIndirectBuffer = dynamic_cast<VulkanArrayBuffer*>(indirectBuffer);
				if (vulkanIndirectBuffer == nullptr) {
					Device()->Log()->Error(
						"VulkanGraphicsPipeline::DrawIndirect - Incompatible indirect buffer provided! ",
						"[File: ", __FILE__, "; Line: ", __LINE__, "]");
					return;
				}

				vkCmdBindPipeline(*commands, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
				vkCmdDrawIndexedIndirect(*commands, *vulkanIndirectBuffer, 0u, 
					static_cast<uint32_t>(drawCount), static_cast<uint32_t>(vulkanIndirectBuffer->ObjectSize()));
				commands->RecordBufferDependency(vulkanIndirectBuffer);
				commands->RecordBufferDependency(this);
			}



			struct VulkanVertexInput::Helpers {
				class SharedIndexBufferHolder : public virtual ObjectCache<Reference<const Object>>::StoredObject {
				private:
					const Reference<GraphicsDevice> m_device;
					SpinLock m_lock;
					ArrayBufferReference<uint32_t> m_buffer;

				public:
					inline SharedIndexBufferHolder(GraphicsDevice* device) : m_device(device) {}
					inline virtual ~SharedIndexBufferHolder() {}
					inline Reference<VulkanArrayBuffer> Get(size_t elementCount) {
						std::unique_lock<SpinLock> lock(m_lock);
						Reference<VulkanArrayBuffer> buffer = m_buffer;
						if (buffer == nullptr || buffer->ObjectCount() < elementCount) {
							size_t actualCount = 1u;
							while (actualCount < static_cast<uint32_t>(elementCount))
								actualCount <<= 1u;
							buffer = m_device->CreateArrayBuffer<uint32_t>(actualCount);
							if (buffer == nullptr)
								m_device->Log()->Error(
									"VulkanVertexInput::Bind - Failed to create shared index buffer!",
									"[File: ", __FILE__, "; Line: ", __LINE__, "]");
							else {
								m_buffer = buffer;
								uint32_t* ptr = m_buffer.Map();
								const uint32_t count = static_cast<uint32_t>(m_buffer->ObjectCount());
								for (uint32_t index = 0u; index < count; index++)
									ptr[index] = index;
								m_buffer->Unmap(true);
							}
						}
						return buffer;
					}
				};

				class SharedBufferCache : public virtual ObjectCache<Reference<const Object>> {
				public:
					static Reference<SharedIndexBufferHolder> Get(GraphicsDevice* device) {
						static SharedBufferCache cache;
						return cache.GetCachedOrCreate(device, false, [&]() -> Reference<SharedIndexBufferHolder> {
							return Object::Instantiate<SharedIndexBufferHolder>(device);
							});
					}
				};
			};

			VulkanVertexInput::VulkanVertexInput(
				VulkanDevice* device,
				const ResourceBinding<Graphics::ArrayBuffer>* const* vertexBuffers, size_t vertexBufferCount,
				const ResourceBinding<Graphics::ArrayBuffer>* indexBuffer) 
				: m_device(device)
				, m_vertexBuffers([&]() -> VertexBindings {
				VertexBindings bindings;
				for (size_t i = 0u; i < vertexBufferCount; i++)
					bindings.Push(vertexBuffers[i]);
				return bindings;
					}())
				, m_indexBuffer(indexBuffer) {
				assert(m_device != nullptr);
				for (size_t i = 0u; i < m_vertexBuffers.Size(); i++)
					if (m_vertexBuffers[i] == nullptr)
						m_device->Log()->Fatal(
							"VulkanVertexInput::VulkanVertexInput - Vertex buffers can not have empty members! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			VulkanVertexInput::~VulkanVertexInput() {}

			void VulkanVertexInput::Bind(CommandBuffer* commandBuffer) {
				VulkanCommandBuffer* commands = dynamic_cast<VulkanCommandBuffer*>(commandBuffer);
				if (commands == nullptr) {
					m_device->Log()->Error(
						"VulkanVertexInput::Bind - Invalid command buffer provided! ",
						"[File: ", __FILE__, "; Line: ", __LINE__, "]");
					return;
				}

				static thread_local std::vector<VkBuffer> vkBuffers;
				static thread_local std::vector<VkDeviceSize> vkOffsets;
				const uint32_t vertexBindingCount = static_cast<uint32_t>(m_vertexBuffers.Size());
				size_t maxBufferSize = 0u;
				
				if (vkBuffers.size() < vertexBindingCount) {
					vkBuffers.resize(m_vertexBuffers.Size());
					vkOffsets.resize(m_vertexBuffers.Size(), 0u);
				}
				
				for (size_t i = 0u; i < vertexBindingCount; i++) {
					VulkanArrayBuffer* const buffer = dynamic_cast<VulkanArrayBuffer*>(m_vertexBuffers[i]->BoundObject());
					if (buffer == nullptr) 
						vkBuffers[i] = VK_NULL_HANDLE;
					else {
						vkBuffers[i] = (*buffer);
						maxBufferSize = Math::Max(maxBufferSize, buffer->ObjectCount());
						commands->RecordBufferDependency(buffer);
					}
				}

				if (vertexBindingCount > 0u)
					vkCmdBindVertexBuffers(*commands, 0u, vertexBindingCount, vkBuffers.data(), vkOffsets.data());

				struct IndexBufferInfo {
					VkBuffer buffer;
					VkIndexType indexType;
				};
				const IndexBufferInfo bufferInfo = [&]() -> IndexBufferInfo {
					Reference<VulkanArrayBuffer> buffer = (m_indexBuffer != nullptr)
						? dynamic_cast<VulkanArrayBuffer*>(m_indexBuffer->BoundObject()) : nullptr;
					if (buffer == nullptr) {
						Reference<Helpers::SharedIndexBufferHolder> sharedIndexBufferHolder = m_sharedIndexBufferHolder;
						if (sharedIndexBufferHolder == nullptr) {
							m_sharedIndexBufferHolder = sharedIndexBufferHolder = Helpers::SharedBufferCache::Get(m_device);
							if (sharedIndexBufferHolder == nullptr)
								return { VK_NULL_HANDLE, VK_INDEX_TYPE_UINT32 };
						}
						buffer = sharedIndexBufferHolder->Get(maxBufferSize);
						if (buffer == nullptr)
							return { VK_NULL_HANDLE, VK_INDEX_TYPE_UINT32 };
					}
					
					const size_t elemSize = buffer->ObjectSize();
					const VkIndexType indexType =
						(elemSize == sizeof(uint32_t)) ? VK_INDEX_TYPE_UINT32 :
						(elemSize == sizeof(uint16_t)) ? VK_INDEX_TYPE_UINT16 : 
						VK_INDEX_TYPE_NONE_KHR;

					if (indexType == VK_INDEX_TYPE_NONE_KHR) {
						m_device->Log()->Error(
							"VulkanVertexInput::Bind - Index buffer HAS TO be an array buffer of uint32_t or uint16_t! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
						return { VK_NULL_HANDLE, VK_INDEX_TYPE_UINT32 };
					}
					else {
						commands->RecordBufferDependency(buffer);
						return { buffer->operator VkBuffer(), indexType };
					}
				}();

				vkCmdBindIndexBuffer(*commands, bufferInfo.buffer, 0, bufferInfo.indexType);
			}
		}
		}
	}
}

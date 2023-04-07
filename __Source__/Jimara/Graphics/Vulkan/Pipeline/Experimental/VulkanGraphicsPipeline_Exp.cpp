#include "VulkanGraphicsPipeline_Exp.h"
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
						size_t bufferId = 0u;
						uint32_t location = 0u;
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
			
			size_t hash = hashOf(key.renderPass.operator->());
			auto includeHash = [&](const auto& v) { 
				hash = Jimara::MergeHashes(hash, hashOf(v)); 
			};

			includeHash(key.vertexShader.operator->());
			includeHash(key.fragmentShader.operator->());
			includeHash(key.blendMode);
			includeHash(key.indexType);

			for (size_t i = 0u; i < key.layoutEntries.Size(); i++) {
				const Jimara::Graphics::Vulkan::VulkanGraphicsPipeline_Identifier::LayoutEntry& entry = key.layoutEntries[i];
				includeHash(entry.bufferId);
				includeHash(entry.location);
				includeHash(entry.inputRate);
				includeHash(entry.attributeType);
			}

			return hash;
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
									entry.bufferId = bufferId;
									entry.location = inputInfo.location;
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

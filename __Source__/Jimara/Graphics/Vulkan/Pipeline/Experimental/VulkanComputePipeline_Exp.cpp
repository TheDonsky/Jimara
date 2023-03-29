#include "VulkanComputePipeline_Exp.h"
#include "../../../../Math/Helpers.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			namespace {
				struct VulkanComputePipeline_Identifier {
					Reference<VulkanDevice> device;
					Reference<const SPIRV_Binary> shader;

					inline bool operator==(const VulkanComputePipeline_Identifier& other)const { 
						return device == other.device && shader == other.shader; 
					}
					inline bool operator!=(const VulkanComputePipeline_Identifier& other)const { 
						return !((*this) == other); 
					}
					inline bool operator<(const VulkanComputePipeline_Identifier& other)const { 
						return device < other.device || (device == other.device && shader < other.shader); 
					}
				};
			}
		}
	}
}

namespace std {
	template<>
	struct hash<Jimara::Graphics::Vulkan::VulkanComputePipeline_Identifier> {
		size_t operator()(const Jimara::Graphics::Vulkan::VulkanComputePipeline_Identifier& key)const {
			return Jimara::MergeHashes(
				std::hash<Jimara::Graphics::Vulkan::VulkanDevice*>()(key.device),
				std::hash<const Jimara::Graphics::SPIRV_Binary*>()(key.shader));
		}
	};
}

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
		namespace Experimental {
			Reference<VulkanComputePipeline> VulkanComputePipeline::Get(VulkanDevice* device, const SPIRV_Binary* computeShader) {
				if (device == nullptr) return nullptr;
				auto fail = [&](const auto&... message) {
					device->Log()->Error("VulkanComputePipeline::Get - ", message...);
					return nullptr;
				};
				if (computeShader == nullptr)
					return fail("Shader not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

#pragma warning(disable: 4250)
				class CachedInstance
					: public virtual VulkanComputePipeline
					, public virtual ObjectCache<VulkanComputePipeline_Identifier>::StoredObject {
				public:
					inline CachedInstance(
						VulkanPipeline::BindingSetBuilder&& bindingSets, 
						VkPipeline pipeline, VulkanShader* shaderModule)
						: VulkanPipeline(std::move(bindingSets))
						, VulkanComputePipeline(pipeline, shaderModule) {}
				};
#pragma warning(default: 4250)

				class Cache : public virtual ObjectCache<VulkanComputePipeline_Identifier> {
				public:
					inline static Reference<VulkanComputePipeline> Get(
						const VulkanComputePipeline_Identifier& identifier, 
						const Function<Reference<CachedInstance>>& createFn) {
						static Cache instance;
						return instance.GetCachedOrCreate(identifier, false, createFn);
					}
				};

				auto create = [&]() -> Reference<CachedInstance> {
					VulkanPipeline::BindingSetBuilder builder(device);

					if (!builder.IncludeShaderBindings(computeShader))
						return fail("Could not configure binding set shape! [File: ", __FILE__, "; Line: ", __LINE__, "]");

					if (!builder.Finish())
						return fail("Could create pipeline layout! [File: ", __FILE__, "; Line: ", __LINE__, "]");

					const Reference<VulkanShader> shaderModule = Object::Instantiate<VulkanShader>(device, computeShader);

					VkComputePipelineCreateInfo createInfo = {};
					{
						createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
						createInfo.pNext = nullptr;
						createInfo.flags = 0;

						createInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
						createInfo.stage.pNext = nullptr;
						createInfo.stage.flags = 0;
						createInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
						createInfo.stage.module = *shaderModule;
						createInfo.stage.pName = computeShader->EntryPoint().c_str();
						createInfo.stage.pSpecializationInfo = nullptr;

						createInfo.layout = builder.PipelineLayout();
						createInfo.basePipelineHandle = VK_NULL_HANDLE;
						createInfo.basePipelineIndex = 0;
					}

					VkPipeline pipeline;
					if (vkCreateComputePipelines(*device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline) != VK_SUCCESS)
						return fail("Failed to create compute pipeline! [File: ", __FILE__, "; Line: ", __LINE__, "]");

					const Reference<CachedInstance> pipelineInstance = new CachedInstance(std::move(builder), pipeline, shaderModule);
					pipelineInstance->ReleaseRef();
					return pipelineInstance;
				};

				return Cache::Get(
					VulkanComputePipeline_Identifier{ device, computeShader },
					Function<Reference<CachedInstance>>::FromCall(&create));
			}

			VulkanComputePipeline::VulkanComputePipeline(VkPipeline pipeline, VulkanShader* shaderModule)
				: m_pipeline(pipeline), m_shaderModule(shaderModule) {
				assert(m_pipeline != nullptr);
				assert(m_shaderModule != nullptr);
			}

			VulkanComputePipeline::~VulkanComputePipeline() {
				if (m_pipeline != VK_NULL_HANDLE)
					vkDestroyPipeline(*Device(), m_pipeline, nullptr);
			}

			void VulkanComputePipeline::Dispatch(CommandBuffer* commandBuffer, const Size3& workGroupCount) {
				VulkanCommandBuffer* vulkanCommandBuffer = dynamic_cast<VulkanCommandBuffer*>(commandBuffer);
				if (vulkanCommandBuffer == nullptr) {
					Device()->Log()->Error("VulkanComputePipeline::Execute - Incompatible command buffer!");
					return;
				}
				if (workGroupCount.x <= 0u || workGroupCount.y <= 0u || workGroupCount.z <= 0u) return;

				VkMemoryBarrier barrier = {};
				{
					barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
					barrier.pNext = nullptr;
					barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
					barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				}
				vkCmdPipelineBarrier(*vulkanCommandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
					| VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					0, 1, &barrier, 0, nullptr, 0, nullptr);

				vkCmdBindPipeline(*vulkanCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
				vkCmdDispatch(*vulkanCommandBuffer, workGroupCount.x, workGroupCount.y, workGroupCount.z);

				vulkanCommandBuffer->RecordBufferDependency(this);
			}
		}
		}
	}
}

#include "VulkanPipeline.h"

#pragma warning(disable: 26812)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			namespace {
				inline static std::vector<std::pair<VkDescriptorSetLayout, uint32_t>> CreateDescriptorSetLayouts(VulkanDevice* device, PipelineDescriptor* descriptor) {
					std::vector<std::pair<VkDescriptorSetLayout, uint32_t>> layouts;
					const size_t setCount = descriptor->BindingSetCount();
					for (size_t setIndex = 0; setIndex < setCount; setIndex++) {
						PipelineDescriptor::BindingSetDescriptor* setDescriptor = descriptor->BindingSet(setIndex);

						static thread_local std::vector<VkDescriptorSetLayoutBinding> bindings;
						bindings.clear();

						auto addBinding = [](PipelineDescriptor::BindingSetDescriptor::BindingInfo info, VkDescriptorType type) {
							VkDescriptorSetLayoutBinding binding = {};
							binding.binding = info.binding;
							binding.descriptorType = type;
							binding.descriptorCount = 1;
							binding.stageFlags = 
								(((info.stages & StageMask(PipelineStage::COMPUTE)) != 0) ? VK_SHADER_STAGE_COMPUTE_BIT : 0) |
								(((info.stages & StageMask(PipelineStage::VERTEX)) != 0) ? VK_SHADER_STAGE_VERTEX_BIT : 0) |
								(((info.stages & StageMask(PipelineStage::FRAGMENT)) != 0) ? VK_SHADER_STAGE_FRAGMENT_BIT : 0);
							binding.pImmutableSamplers = nullptr;
							bindings.push_back(binding);
						};

						{
							size_t count = setDescriptor->ConstantBufferCount();
							for (size_t i = 0; i < count; i++)
								addBinding(setDescriptor->ConstantBufferInfo(i), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
						}
						{
							size_t count = setDescriptor->TextureSamplerCount();
							for (size_t i = 0; i < count; i++)
								addBinding(setDescriptor->TextureSamplerInfo(i), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
						}
						if (bindings.size() <= 0) continue;

						VkDescriptorSetLayoutCreateInfo layoutInfo = {};
						{
							layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
							layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
							layoutInfo.pBindings = bindings.data();
						}
						VkDescriptorSetLayout layout;
						if (vkCreateDescriptorSetLayout(*device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
							device->Log()->Fatal("VulkanPipeline - Failed to create descriptor set layout!");
						}
						else layouts.push_back(std::make_pair(layout, setDescriptor->SetId()));
					}
					return layouts;
				}

				inline static VkDescriptorPool CreateDescriptorPool(VulkanDevice* device, PipelineDescriptor* descriptor, size_t maxInFlightCommandBuffers) {
					VkDescriptorPoolSize sizes[2];

					uint32_t sizeCount = 0;
					uint32_t constantBufferCount = 0;
					uint32_t textureSamplerCount = 0;

					const size_t setCount = descriptor->BindingSetCount();
					for (size_t setIndex = 0; setIndex < setCount; setIndex++) {
						const PipelineDescriptor::BindingSetDescriptor* setDescriptor = descriptor->BindingSet(setIndex);
						constantBufferCount += static_cast<uint32_t>(setDescriptor->ConstantBufferCount());
						textureSamplerCount += static_cast<uint32_t>(setDescriptor->TextureSamplerCount());
					}

					{
						VkDescriptorPoolSize& size = sizes[sizeCount];
						size = {};
						size.descriptorCount = constantBufferCount * static_cast<uint32_t>(maxInFlightCommandBuffers);
						size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						if (size.descriptorCount > 0) sizeCount++;
					}
					{
						VkDescriptorPoolSize& size = sizes[sizeCount];
						size = {};
						size.descriptorCount = textureSamplerCount * static_cast<uint32_t>(maxInFlightCommandBuffers);
						size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						if (size.descriptorCount > 0) sizeCount++;
					}
					if ((constantBufferCount + textureSamplerCount) <= 0) return VK_NULL_HANDLE;

					VkDescriptorPoolCreateInfo createInfo = {};
					{
						createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
						createInfo.poolSizeCount = sizeCount;
						createInfo.pPoolSizes = (sizeCount > 0) ? sizes : nullptr;
						createInfo.maxSets = static_cast<uint32_t>(setCount * maxInFlightCommandBuffers);
					}
					VkDescriptorPool pool;
					if (vkCreateDescriptorPool(*device, &createInfo, nullptr, &pool) != VK_SUCCESS) {
						pool = VK_NULL_HANDLE;
						device->Log()->Fatal("VulkanPipeline - Failed to create descriptor pool!");
					}
					return pool;
				}

				inline static std::vector<VkDescriptorSet> CreateDescriptorSets(
					VulkanDevice* device, PipelineDescriptor* descriptor, size_t maxInFlightCommandBuffers
					, VkDescriptorPool pool, const std::vector<std::pair<VkDescriptorSetLayout, uint32_t>>& setLayouts) {

					static thread_local std::vector<VkDescriptorSetLayout> layouts;
					const uint32_t setCount = static_cast<uint32_t>(maxInFlightCommandBuffers * setLayouts.size());
					if (setCount > 0) {
						if (layouts.size() < setCount)
							layouts.resize(setCount);

						VkDescriptorSetLayout* ptr = layouts.data();
						for (size_t i = 0; i < maxInFlightCommandBuffers; i++) {
							for (size_t j = 0; j < setLayouts.size(); j++)
								ptr[j] = setLayouts[j].first;
							ptr += setLayouts.size();
						}
					}
					else return std::vector<VkDescriptorSet>();

					std::vector<VkDescriptorSet> sets(setCount);

					VkDescriptorSetAllocateInfo allocInfo = {};
					{
						allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
						allocInfo.descriptorPool = pool;
						allocInfo.descriptorSetCount = setCount;
						allocInfo.pSetLayouts = layouts.data();
					}
					if (vkAllocateDescriptorSets(*device, &allocInfo, sets.data()) != VK_SUCCESS) {
						device->Log()->Fatal("VulkanPipeline - Failed to allocate descriptor sets!");
						sets.clear();
					}
					return sets;
				}

				inline static VkPipelineLayout CreateVulkanPipelineLayout(VulkanDevice* device, const VkDescriptorSetLayout* layouts, uint32_t layoutCount) {
					VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
					{
						pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
						pipelineLayoutInfo.setLayoutCount = layoutCount;
						pipelineLayoutInfo.pSetLayouts = layouts;
						pipelineLayoutInfo.pushConstantRangeCount = 0;
					}
					VkPipelineLayout pipelineLayout;
					if (vkCreatePipelineLayout(*device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
						device->Log()->Fatal("VulkanPipeline - Failed to create pipeline layout!");
						return VK_NULL_HANDLE;
					}
					return pipelineLayout;
				}


				inline static void PrepareCache(const PipelineDescriptor* descriptor, size_t maxInFlightCommandBuffers
					, std::vector<Reference<VulkanStaticImageSampler>>& samplerRefs) {
					size_t textureSamplerCount = 0;
					const size_t setCount = descriptor->BindingSetCount();
					for (size_t setIndex = 0; setIndex < setCount; setIndex++) {
						const PipelineDescriptor::BindingSetDescriptor* setDescriptor = descriptor->BindingSet(setIndex);
						textureSamplerCount += static_cast<uint32_t>(setDescriptor->TextureSamplerCount());
					}

					samplerRefs.resize(textureSamplerCount * maxInFlightCommandBuffers);
				}
			}

			VulkanPipeline::VulkanPipeline(VulkanDevice* device, PipelineDescriptor* descriptor, size_t maxInFlightCommandBuffers)
				: m_device(device), m_descriptor(descriptor), m_commandBufferCount(maxInFlightCommandBuffers)
				, m_descriptorPool(VK_NULL_HANDLE), m_pipelineLayout(VK_NULL_HANDLE) {
				
				m_descriptorSetLayouts = CreateDescriptorSetLayouts(m_device, m_descriptor);
				m_descriptorPool = CreateDescriptorPool(m_device, m_descriptor, m_commandBufferCount);
				m_descriptorSets = CreateDescriptorSets(m_device, m_descriptor, m_commandBufferCount, m_descriptorPool, m_descriptorSetLayouts);

				static thread_local std::vector<VkDescriptorSetLayout> layouts;
				static thread_local std::vector<size_t> layoutIndices;

				uint32_t setCount = 0;
				for (size_t i = 0; i < m_descriptorSetLayouts.size(); i++) {
					uint32_t minCount = m_descriptorSetLayouts[i].second + 1;
					if (setCount < minCount) setCount = minCount;
				}

				if (layouts.size() < setCount) {
					layouts.resize(setCount);
					layoutIndices.resize(setCount);
				}

				const uint32_t NO_DESCRIPTOR_SET = static_cast<uint32_t>(m_descriptorSetLayouts.size());
				for (size_t i = 0; i < setCount; i++) {
					layouts[i] = VK_NULL_HANDLE;
					layoutIndices[i] = NO_DESCRIPTOR_SET;
				}

				for (size_t i = 0; i < m_descriptorSetLayouts.size(); i++) {
					const std::pair<VkDescriptorSetLayout, uint32_t>& layout = m_descriptorSetLayouts[i];
					layouts[layout.second] = layout.first;
					layoutIndices[layout.second] = i;
				}

				m_pipelineLayout = CreateVulkanPipelineLayout(m_device, setCount > 0 ? layouts.data() : nullptr, setCount);


				for (size_t commandBuffer = 0; commandBuffer < maxInFlightCommandBuffers; commandBuffer++) {
					static thread_local std::vector<DescriptorBindingRange> ranges;
					static thread_local DescriptorBindingRange currentRange;
					currentRange.start = NO_DESCRIPTOR_SET;

					const size_t BASE_DESCRIPTOR_INDEX = (m_descriptorSetLayouts.size() * commandBuffer);
					for (uint32_t i = 0; i < setCount; i++) {
						size_t layoutIndex = layoutIndices[i];
						if (layoutIndex < NO_DESCRIPTOR_SET) {
							if (currentRange.start >= NO_DESCRIPTOR_SET)
								currentRange.start = i;
							currentRange.sets.push_back(m_descriptorSets[BASE_DESCRIPTOR_INDEX + layoutIndex]);
						}
						else if (currentRange.start < NO_DESCRIPTOR_SET) {
							ranges.push_back(currentRange);
							currentRange.sets.clear();
							currentRange.start = NO_DESCRIPTOR_SET;
						}
					}
					if (currentRange.start < NO_DESCRIPTOR_SET) {
						ranges.push_back(currentRange);
						currentRange.sets.clear();
					}

					m_bindingRanges.push_back(ranges);
					ranges.clear();
				}

				PrepareCache(m_descriptor, m_commandBufferCount, m_descriptorCache.samplers);
			}

			VulkanPipeline::~VulkanPipeline() {
				m_bindingRanges.clear();
				if (m_pipelineLayout != VK_NULL_HANDLE) {
					vkDestroyPipelineLayout(*m_device, m_pipelineLayout, nullptr);
					m_pipelineLayout = VK_NULL_HANDLE;
				}
				if (m_descriptorPool != VK_NULL_HANDLE) {
					vkDestroyDescriptorPool(*m_device, m_descriptorPool, nullptr);
					m_descriptorPool = VK_NULL_HANDLE;
				}
				for (size_t i = 0; i < m_descriptorSetLayouts.size(); i++)
					vkDestroyDescriptorSetLayout(*m_device, m_descriptorSetLayouts[i].first, nullptr);
				m_descriptorSetLayouts.clear();
			}

			VkPipelineLayout VulkanPipeline::PipelineLayout()const { 
				return m_pipelineLayout; 
			}

			namespace {
				inline static VkDescriptorSet* DescriptorSets(
					std::vector<VkDescriptorSet>& sets, const std::vector<std::pair<VkDescriptorSetLayout, uint32_t>>& layouts, size_t commandBufferId) {
					return (sets.data() + (layouts.size() * commandBufferId));
				}
			}

			void VulkanPipeline::UpdateDescriptors(VulkanCommandRecorder* recorder) {
				static thread_local std::vector<VkWriteDescriptorSet> updates;
				static thread_local std::vector<VkDescriptorImageInfo> samplerInfos;
				if (samplerInfos.size() < m_descriptorCache.samplers.size())
					samplerInfos.resize(m_descriptorCache.samplers.size());

				const size_t commandBufferIndex = recorder->CommandBufferIndex();
				VkDescriptorSet* sets = m_descriptorSets.data() + (m_descriptorSetLayouts.size() * recorder->CommandBufferIndex());
				{
					size_t samplerCacheIndex = commandBufferIndex;
					const size_t setCount = m_descriptor->BindingSetCount();
					for (size_t setIndex = 0; setIndex < setCount; setIndex++) {
						PipelineDescriptor::BindingSetDescriptor* setDescriptor = m_descriptor->BindingSet(setIndex);
						VkDescriptorSet set = sets[setIndex];

						size_t samplerCount = setDescriptor->TextureSamplerCount();
						for (size_t samplerId = 0; samplerId < samplerCount; samplerId++) {
							Reference<VulkanImageSampler> sampler = setDescriptor->Sampler(samplerId);
							Reference<VulkanStaticImageSampler> staticSampler = (sampler != nullptr) ? sampler->GetStaticHandle(recorder) : nullptr;
							Reference<VulkanStaticImageSampler>& cachedSampler = m_descriptorCache.samplers[samplerCacheIndex];
							if (cachedSampler != staticSampler) {
								cachedSampler = staticSampler;

								VkDescriptorImageInfo& samplerInfo = samplerInfos[samplerCacheIndex];
								samplerInfo = {};
								samplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
								samplerInfo.imageView = *dynamic_cast<VulkanStaticImageView*>(cachedSampler->TargetView());
								samplerInfo.sampler = *cachedSampler;

								VkWriteDescriptorSet write = {};
								write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
								write.dstSet = set;
								write.dstBinding = setDescriptor->TextureSamplerInfo(samplerCacheIndex).binding;
								write.dstArrayElement = 0;
								write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
								write.descriptorCount = 1;
								write.pImageInfo = &samplerInfo;

								updates.push_back(write);
							}
							samplerCacheIndex += m_commandBufferCount;
						}
					}
				}
				if (updates.size() > 0) {
					vkUpdateDescriptorSets(*m_device, static_cast<uint32_t>(updates.size()), updates.data(), 0, nullptr);
					updates.clear();
				}
			}

			void VulkanPipeline::SetDescriptors(VulkanCommandRecorder* recorder, VkPipelineBindPoint bindPoint) {
				const size_t commandBufferIndex = recorder->CommandBufferIndex();
				const VkCommandBuffer commandBuffer = recorder->CommandBuffer();

				const std::vector<DescriptorBindingRange>& ranges = m_bindingRanges[commandBufferIndex];

				for (size_t i = 0; i < ranges.size(); i++) {
					const DescriptorBindingRange& range = ranges[i];
					vkCmdBindDescriptorSets(commandBuffer, bindPoint, m_pipelineLayout, range.start, static_cast<uint32_t>(range.sets.size()), range.sets.data(), 0, nullptr);
				}
			}
		}
	}
}
#pragma warning(default: 26812)

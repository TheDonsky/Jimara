#include "VulkanPipeline.h"

#pragma warning(disable: 26812)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			namespace {
				inline static std::vector<VkDescriptorSetLayout> CreateDescriptorSetLayouts(VulkanDevice* device, const PipelineDescriptor* descriptor) {
					std::vector<VkDescriptorSetLayout> layouts;
					const size_t setCount = descriptor->BindingSetCount();
					for (size_t setIndex = 0; setIndex < setCount; setIndex++) {
						const PipelineDescriptor::BindingSetDescriptor* setDescriptor = descriptor->BindingSet(setIndex);

						if (setDescriptor->IsBindlessArrayBufferArray()) {
							VkDescriptorSetLayout layout = VulkanBindlessInstance<ArrayBuffer>::CreateDescriptorSetLayout(device);
							if (layout == VK_NULL_HANDLE)
								device->Log()->Fatal("Vulkan Pipeline - Failed to create descriptor set layout for VulkanBindlessInstance<ArrayBuffer>!");
							else layouts.push_back(layout);
							continue;
						}
						else if (setDescriptor->IsBindlessTextureSamplerArray()) {
							VkDescriptorSetLayout layout = VulkanBindlessInstance<TextureSampler>::CreateDescriptorSetLayout(device);
							if (layout == VK_NULL_HANDLE)
								device->Log()->Fatal("Vulkan Pipeline - Failed to create descriptor set layout for VulkanBindlessInstance<TextureSampler>!");
							else layouts.push_back(layout);
							continue;
						}

						static thread_local std::vector<VkDescriptorSetLayoutBinding> bindings;
						bindings.clear();

						auto addBinding = [](const PipelineDescriptor::BindingSetDescriptor::BindingInfo info, VkDescriptorType type) {
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
							const size_t count = setDescriptor->ConstantBufferCount();
							for (size_t i = 0; i < count; i++)
								addBinding(setDescriptor->ConstantBufferInfo(i), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
						}
						{
							const size_t count = setDescriptor->StructuredBufferCount();
							for (size_t i = 0; i < count; i++)
								addBinding(setDescriptor->StructuredBufferInfo(i), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
						}
						{
							const size_t count = setDescriptor->TextureSamplerCount();
							for (size_t i = 0; i < count; i++)
								addBinding(setDescriptor->TextureSamplerInfo(i), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
						}
						{
							const size_t count = setDescriptor->TextureViewCount();
							for (size_t i = 0; i < count; i++)
								addBinding(setDescriptor->TextureViewInfo(i), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
						}

						VkDescriptorSetLayoutCreateInfo layoutInfo = {};
						{
							layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
							layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
							layoutInfo.pBindings = bindings.data();
						}
						VkDescriptorSetLayout layout;
						std::unique_lock<std::mutex> lock(device->PipelineCreationLock());
						if (vkCreateDescriptorSetLayout(*device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
							device->Log()->Fatal("VulkanPipeline - Failed to create descriptor set layout!");
							layout = VK_NULL_HANDLE;
						}
						layouts.push_back(layout);
					}
					return layouts;
				}

				inline static VkPipelineLayout CreateVulkanPipelineLayout(VulkanDevice* device, const std::vector<VkDescriptorSetLayout>& setLayouts) {
					VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
					{
						pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
						pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
						pipelineLayoutInfo.pSetLayouts = (setLayouts.size() > 0) ? setLayouts.data() : nullptr;
						pipelineLayoutInfo.pushConstantRangeCount = 0;
					}
					VkPipelineLayout pipelineLayout;
					std::unique_lock<std::mutex> lock(device->PipelineCreationLock());
					if (vkCreatePipelineLayout(*device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
						device->Log()->Fatal("VulkanPipeline - Failed to create pipeline layout!");
						return VK_NULL_HANDLE;
					}
					return pipelineLayout;
				}


				inline static VkDescriptorPool CreateDescriptorPool(VulkanDevice* device, const PipelineDescriptor* descriptor, size_t maxInFlightCommandBuffers) {
					VkDescriptorPoolSize sizes[4];

					uint32_t sizeCount = 0u;
					uint32_t constantBufferCount = 0u;
					uint32_t structuredBufferCount = 0u;
					uint32_t textureSamplerCount = 0u;
					uint32_t textureViewCount = 0u;

					const size_t setCount = descriptor->BindingSetCount();
					for (size_t setIndex = 0; setIndex < setCount; setIndex++) {
						const PipelineDescriptor::BindingSetDescriptor* setDescriptor = descriptor->BindingSet(setIndex);
						if (setDescriptor->SetByEnvironment() || setDescriptor->IsBindlessArrayBufferArray() || setDescriptor->IsBindlessTextureSamplerArray()) continue;

						constantBufferCount += static_cast<uint32_t>(setDescriptor->ConstantBufferCount());
						structuredBufferCount += static_cast<uint32_t>(setDescriptor->StructuredBufferCount());
						textureSamplerCount += static_cast<uint32_t>(setDescriptor->TextureSamplerCount());
						textureViewCount += static_cast<uint32_t>(setDescriptor->TextureViewCount());
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
						size.descriptorCount = structuredBufferCount * static_cast<uint32_t>(maxInFlightCommandBuffers);
						size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						if (size.descriptorCount > 0) sizeCount++;
					}
					{
						VkDescriptorPoolSize& size = sizes[sizeCount];
						size = {};
						size.descriptorCount = textureSamplerCount * static_cast<uint32_t>(maxInFlightCommandBuffers);
						size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						if (size.descriptorCount > 0) sizeCount++;
					}
					{
						VkDescriptorPoolSize& size = sizes[sizeCount];
						size = {};
						size.descriptorCount = textureViewCount * static_cast<uint32_t>(maxInFlightCommandBuffers);
						size.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
						if (size.descriptorCount > 0) sizeCount++;
					}
					if (sizeCount <= 0u) return VK_NULL_HANDLE;

					VkDescriptorPoolCreateInfo createInfo = {};
					{
						createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
						createInfo.poolSizeCount = sizeCount;
						createInfo.pPoolSizes = (sizeCount > 0) ? sizes : nullptr;
						createInfo.maxSets = static_cast<uint32_t>(setCount * maxInFlightCommandBuffers);
					}
					VkDescriptorPool pool;
					std::unique_lock<std::mutex> lock(device->PipelineCreationLock());
					if (vkCreateDescriptorPool(*device, &createInfo, nullptr, &pool) != VK_SUCCESS) {
						pool = VK_NULL_HANDLE;
						device->Log()->Fatal("VulkanPipeline - Failed to create descriptor pool!");
					}
					return pool;
				}

				inline static std::vector<VkDescriptorSet> CreateDescriptorSets(
					VulkanDevice* device, PipelineDescriptor* descriptor, size_t maxInFlightCommandBuffers
					, VkDescriptorPool pool, const std::vector<VkDescriptorSetLayout>& setLayouts) {
					if (pool == VK_NULL_HANDLE) return {};

					static thread_local std::vector<VkDescriptorSetLayout> layouts;

					const uint32_t maxSetCount = static_cast<uint32_t>(maxInFlightCommandBuffers * setLayouts.size());
					if (layouts.size() < maxSetCount)
						layouts.resize(maxSetCount);

					uint32_t setCountPerCommandBuffer = 0;
					for (size_t i = 0; i < setLayouts.size(); i++) {
						const PipelineDescriptor::BindingSetDescriptor* bindingSet = descriptor->BindingSet(i);
						if (!(bindingSet->SetByEnvironment() || bindingSet->IsBindlessArrayBufferArray() || bindingSet->IsBindlessTextureSamplerArray())) {
							layouts[setCountPerCommandBuffer] = setLayouts[i];
							setCountPerCommandBuffer++;
						}
					}

					for (size_t i = 1; i < maxInFlightCommandBuffers; i++)
						for (size_t j = 0; j < setCountPerCommandBuffer; j++)
							layouts[(i * setCountPerCommandBuffer) + j] = layouts[j];
					
					const uint32_t setCount = static_cast<uint32_t>(maxInFlightCommandBuffers * setCountPerCommandBuffer);
					if (setCount <= 0) return std::vector<VkDescriptorSet>();

					std::vector<VkDescriptorSet> sets(setCount);

					VkDescriptorSetAllocateInfo allocInfo = {};
					{
						allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
						allocInfo.descriptorPool = pool;
						allocInfo.descriptorSetCount = setCount;
						allocInfo.pSetLayouts = layouts.data();
					}
					std::unique_lock<std::mutex> lock(device->PipelineCreationLock());
					if (vkAllocateDescriptorSets(*device, &allocInfo, sets.data()) != VK_SUCCESS) {
						device->Log()->Fatal("VulkanPipeline - Failed to allocate descriptor sets!");
						sets.clear();
					}
					return sets;
				}


				inline static void PrepareCache(const PipelineDescriptor* descriptor, size_t maxInFlightCommandBuffers
					, std::vector<Reference<VulkanPipelineConstantBuffer>>& constantBuffers
					, std::vector<Reference<VulkanPipelineConstantBuffer>>& boundBuffers
					, std::vector<Reference<VulkanArrayBuffer>>& structuredBuffers
					, std::vector<Reference<VulkanTextureSampler>>& samplerRefs
					, std::vector<Reference<VulkanTextureView>>& viewRefs) {
					
					size_t constantBufferCount = 0u;
					size_t structuredBufferCount = 0u;
					size_t textureSamplerCount = 0u;
					size_t textureViewCount = 0u;
					
					const size_t setCount = descriptor->BindingSetCount();
					for (size_t setIndex = 0; setIndex < setCount; setIndex++) {
						const PipelineDescriptor::BindingSetDescriptor* setDescriptor = descriptor->BindingSet(setIndex);
						if (setDescriptor->SetByEnvironment() || setDescriptor->IsBindlessArrayBufferArray() || setDescriptor->IsBindlessTextureSamplerArray()) continue;
						constantBufferCount += setDescriptor->ConstantBufferCount();
						structuredBufferCount += setDescriptor->StructuredBufferCount();
						textureSamplerCount += setDescriptor->TextureSamplerCount();
						textureViewCount += setDescriptor->TextureViewCount();
					}

					constantBuffers.resize(constantBufferCount);
					boundBuffers.resize(constantBufferCount * maxInFlightCommandBuffers);
					structuredBuffers.resize(structuredBufferCount * maxInFlightCommandBuffers);
					samplerRefs.resize(textureSamplerCount * maxInFlightCommandBuffers);
					viewRefs.resize(textureViewCount * maxInFlightCommandBuffers);
				}
			}

			VulkanPipeline::VulkanPipeline(VulkanDevice* device, PipelineDescriptor* descriptor, size_t maxInFlightCommandBuffers)
				: m_device(device), m_descriptor(descriptor), m_commandBufferCount(maxInFlightCommandBuffers)
				, m_descriptorPool(VK_NULL_HANDLE), m_pipelineLayout(VK_NULL_HANDLE) {
				m_descriptorSetLayouts = CreateDescriptorSetLayouts(m_device, m_descriptor);
				m_pipelineLayout = CreateVulkanPipelineLayout(m_device, m_descriptorSetLayouts);

				m_descriptorPool = CreateDescriptorPool(m_device, m_descriptor, m_commandBufferCount);
				m_descriptorSets = CreateDescriptorSets(m_device, m_descriptor, m_commandBufferCount, m_descriptorPool, m_descriptorSetLayouts);

				PrepareCache(m_descriptor, m_commandBufferCount, 
					m_descriptorCache.constantBuffers, m_descriptorCache.boundBuffers, 
					m_descriptorCache.structuredBuffers, m_descriptorCache.samplers, m_descriptorCache.views);

				m_bindingRanges.resize(m_commandBufferCount);
				if (m_commandBufferCount > 0) {
					const size_t setsPerCommandBuffer = (m_descriptorSets.size() / m_commandBufferCount);
					const size_t setCount = descriptor->BindingSetCount();
					bool shouldStartNew = true;
					uint32_t setId = 0;
					for (size_t i = 0; i < setCount; i++) {
						const PipelineDescriptor::BindingSetDescriptor* setDescriptor = descriptor->BindingSet(i);
						if (setDescriptor->SetByEnvironment()) {
							shouldStartNew = true;
							continue;
						}
						else if (setDescriptor->IsBindlessArrayBufferArray() || setDescriptor->IsBindlessTextureSamplerArray()) {
							m_bindlessCache.push_back({});
							shouldStartNew = true;
							continue;
						}
						if (shouldStartNew) {
							for (size_t buffer = 0; buffer < m_commandBufferCount; buffer++) {
								DescriptorBindingRange range;
								range.start = static_cast<uint32_t>(i);
								m_bindingRanges[buffer].push_back(range);
							}
							shouldStartNew = false;
						}
						for (size_t buffer = 0; buffer < m_commandBufferCount; buffer++)
							m_bindingRanges[buffer].back().sets.push_back(m_descriptorSets[(setsPerCommandBuffer * buffer) + setId]);
						setId++;
					}
				}
			}

			VulkanPipeline::~VulkanPipeline() {
				std::unique_lock<std::mutex> lock(m_device->PipelineCreationLock());
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
					vkDestroyDescriptorSetLayout(*m_device, m_descriptorSetLayouts[i], nullptr);
				m_descriptorSetLayouts.clear();
			}

			VulkanDevice* VulkanPipeline::Device()const {
				return m_device;
			}

			VkPipelineLayout VulkanPipeline::PipelineLayout()const { 
				return m_pipelineLayout; 
			}

			PipelineDescriptor* VulkanPipeline::Descriptor()const {
				return m_descriptor;
			}

			void VulkanPipeline::UpdateDescriptors(const CommandBufferInfo& bufferInfo) {
				static thread_local std::vector<VkWriteDescriptorSet> updates;

				static thread_local std::vector<VkDescriptorBufferInfo> bufferInfos;
				if (bufferInfos.size() < m_descriptorCache.boundBuffers.size())
					bufferInfos.resize(m_descriptorCache.boundBuffers.size());

				static thread_local std::vector<VkDescriptorBufferInfo> structuredBufferInfos;
				if (structuredBufferInfos.size() < m_descriptorCache.structuredBuffers.size())
					structuredBufferInfos.resize(m_descriptorCache.structuredBuffers.size());

				static thread_local std::vector<VkDescriptorImageInfo> samplerInfos;
				if (samplerInfos.size() < (m_descriptorCache.samplers.size() + m_descriptorCache.views.size()))
					samplerInfos.resize(m_descriptorCache.samplers.size() + m_descriptorCache.views.size());

				if(m_commandBufferCount > 0) {
					const size_t commandBufferIndex = bufferInfo.inFlightBufferId;
					const size_t setsPerCommandBuffer = (m_descriptorSets.size() / m_commandBufferCount);
					VkDescriptorSet* sets = m_descriptorSets.data() + (setsPerCommandBuffer * commandBufferIndex);

					VulkanCommandBuffer* commandBuffer = dynamic_cast<VulkanCommandBuffer*>(bufferInfo.commandBuffer);

					size_t constantBufferId = 0;
					size_t boundBufferId = commandBufferIndex;
					auto addConstantBuffers = [&](const PipelineDescriptor::BindingSetDescriptor* setDescriptor, size_t setIndex) {
						const size_t cbufferCount = setDescriptor->ConstantBufferCount();
						for (size_t cbufferId = 0; cbufferId < cbufferCount; cbufferId++) {
							Reference<VulkanConstantBuffer> buffer = setDescriptor->ConstantBuffer(cbufferId);
							Reference<VulkanPipelineConstantBuffer>& boundBuffer = m_descriptorCache.boundBuffers[boundBufferId];
							
							if (boundBuffer == nullptr || boundBuffer->TargetBuffer() != buffer) {
								{
									Reference<VulkanPipelineConstantBuffer>& pipelineBuffer = m_descriptorCache.constantBuffers[constantBufferId];
									if (pipelineBuffer == nullptr || pipelineBuffer->TargetBuffer() != buffer)
										pipelineBuffer = (buffer == nullptr) ? nullptr : Object::Instantiate<VulkanPipelineConstantBuffer>(m_device, buffer, m_commandBufferCount);
									boundBuffer = pipelineBuffer;
								}

								VkDescriptorBufferInfo& bufferInfo = bufferInfos[(commandBufferIndex * m_descriptorCache.constantBuffers.size()) + constantBufferId];
								bufferInfo = {};
								std::pair<VkBuffer, VkDeviceSize> bufferAndOffset = (boundBuffer != nullptr)
									? boundBuffer->GetBuffer(commandBufferIndex) : std::pair<VkBuffer, VkDeviceSize>(VK_NULL_HANDLE, 0);
								bufferInfo.buffer = bufferAndOffset.first;
								bufferInfo.offset = bufferAndOffset.second;
								bufferInfo.range = buffer->ObjectSize();

								VkWriteDescriptorSet write = {};
								write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
								write.dstSet = sets[setIndex];
								write.dstBinding = setDescriptor->ConstantBufferInfo(cbufferId).binding;
								write.dstArrayElement = 0;
								write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
								write.descriptorCount = 1;
								write.pBufferInfo = &bufferInfo;

								updates.push_back(write);
							}
							else boundBuffer->GetBuffer(commandBufferIndex);

							if (boundBuffer != nullptr) commandBuffer->RecordBufferDependency(boundBuffer);

							constantBufferId++;
							boundBufferId += m_commandBufferCount;
						}
					};

					size_t structuredBufferId = commandBufferIndex;
					auto addStructuredBuffers = [&](const PipelineDescriptor::BindingSetDescriptor* setDescriptor, VkDescriptorSet set) {
						const size_t structuredBufferCount = setDescriptor->StructuredBufferCount();
						for (size_t bufferId = 0; bufferId < structuredBufferCount; bufferId++) {
							Reference<VulkanArrayBuffer> buffer = setDescriptor->StructuredBuffer(bufferId);
							VkBuffer vulkanBufferHandle = (buffer != nullptr) ? buffer->operator VkBuffer() : VK_NULL_HANDLE;
							Reference<VulkanArrayBuffer>& cachedBuffer = m_descriptorCache.structuredBuffers[structuredBufferId];
							if (cachedBuffer != buffer) {
								cachedBuffer = buffer;

								VkDescriptorBufferInfo& bufferInfo = structuredBufferInfos[structuredBufferId];
								bufferInfo = {};
								bufferInfo.buffer = vulkanBufferHandle;
								bufferInfo.offset = 0;
								bufferInfo.range = VK_WHOLE_SIZE;

								VkWriteDescriptorSet write = {};
								write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
								write.dstSet = set;
								write.dstBinding = setDescriptor->StructuredBufferInfo(bufferId).binding;
								write.dstArrayElement = 0;
								write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
								write.descriptorCount = 1;
								write.pBufferInfo = &bufferInfo;

								updates.push_back(write);
							}
							structuredBufferId += m_commandBufferCount;
							if (cachedBuffer != nullptr) commandBuffer->RecordBufferDependency(cachedBuffer);
						}
					};

					size_t samplerCacheIndex = commandBufferIndex;
					auto addSamplers = [&](const PipelineDescriptor::BindingSetDescriptor* setDescriptor, VkDescriptorSet set) {
						const size_t samplerCount = setDescriptor->TextureSamplerCount();
						for (size_t samplerId = 0; samplerId < samplerCount; samplerId++) {
							Reference<VulkanTextureSampler> sampler = setDescriptor->Sampler(samplerId);
							Reference<VulkanTextureSampler>& cachedSampler = m_descriptorCache.samplers[samplerCacheIndex];
							if (cachedSampler != sampler) {
								cachedSampler = sampler;

								VkDescriptorImageInfo& samplerInfo = samplerInfos[samplerCacheIndex];
								samplerInfo = {};
								samplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
								samplerInfo.imageView = *dynamic_cast<VulkanTextureView*>(cachedSampler->TargetView());
								samplerInfo.sampler = *cachedSampler;

								VkWriteDescriptorSet write = {};
								write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
								write.dstSet = set;
								write.dstBinding = setDescriptor->TextureSamplerInfo(samplerId).binding;
								write.dstArrayElement = 0;
								write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
								write.descriptorCount = 1;
								write.pImageInfo = &samplerInfo;

								updates.push_back(write);
							}
							samplerCacheIndex += m_commandBufferCount;
							if (cachedSampler != nullptr) commandBuffer->RecordBufferDependency(cachedSampler);
						}
					};

					size_t viewCacheIndex = commandBufferIndex;
					auto addViews = [&](const PipelineDescriptor::BindingSetDescriptor* setDescriptor, VkDescriptorSet set) {
						const size_t viewCount = setDescriptor->TextureViewCount();
						for (size_t viewId = 0; viewId < viewCount; viewId++) {
							Reference<VulkanTextureView> view = setDescriptor->View(viewId);
							Reference<VulkanTextureView>& cachedView = m_descriptorCache.views[viewCacheIndex];
							if (view != cachedView) {
								cachedView = view;

								VkDescriptorImageInfo& viewInfo = samplerInfos[m_descriptorCache.samplers.size() + viewCacheIndex];
								viewInfo = {};
								viewInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
								viewInfo.imageView = *view;
								viewInfo.sampler = nullptr;

								VkWriteDescriptorSet write = {};
								write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
								write.dstSet = set;
								write.dstBinding = setDescriptor->TextureViewInfo(viewId).binding;
								write.dstArrayElement = 0;
								write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
								write.descriptorCount = 1;
								write.pImageInfo = &viewInfo;

								updates.push_back(write);
							}
							viewCacheIndex += m_commandBufferCount;
							if (view != nullptr) commandBuffer->RecordBufferDependency(view);
						}
					};

					size_t bindlessId = 0;
					auto addBindlessBinding = [&](const auto& set, size_t setId) {
						if (set == nullptr) return;
						BindlessSetBinding binding;
						binding.setId = static_cast<uint32_t>(setId);
						binding.descriptorSet = set->GetDescriptorSet(commandBufferIndex);
						binding.descriptorSetLock = &set->GetDescriptorSetLock(commandBufferIndex);
						binding.bindlessInstance = set;
						m_bindlessCache[bindlessId] = binding;
						bindlessId++;
						commandBuffer->RecordBufferDependency(set);
					};

					const size_t setCount = m_descriptor->BindingSetCount();
					size_t setIndex = 0;
					for (size_t i = 0; i < setCount; i++) {
						const PipelineDescriptor::BindingSetDescriptor* setDescriptor = m_descriptor->BindingSet(i);
						if (setDescriptor->SetByEnvironment()) continue;
						else if (setDescriptor->IsBindlessArrayBufferArray()) {
							Reference<VulkanBindlessInstance<ArrayBuffer>> set = setDescriptor->BindlessArrayBuffers();
							addBindlessBinding(set, i);
						}
						else if (setDescriptor->IsBindlessTextureSamplerArray()) {
							Reference<VulkanBindlessInstance<TextureSampler>> set = setDescriptor->BindlessTextureSamplers();
							addBindlessBinding(set, i);
						}
						else {
							VkDescriptorSet set = sets[setIndex];
							addConstantBuffers(setDescriptor, setIndex);
							addStructuredBuffers(setDescriptor, set);
							addSamplers(setDescriptor, set);
							addViews(setDescriptor, set);
							setIndex++;
						}
					}
				}
				if (updates.size() > 0) {
					std::unique_lock<std::mutex> lock(m_descriptorUpdateLock);
					vkUpdateDescriptorSets(*m_device, static_cast<uint32_t>(updates.size()), updates.data(), 0, nullptr);
					updates.clear();
				}
			}

			void VulkanPipeline::BindDescriptors(const CommandBufferInfo& bufferInfo, VkPipelineBindPoint bindPoint) {
				const VkCommandBuffer commandBuffer = *dynamic_cast<VulkanCommandBuffer*>(bufferInfo.commandBuffer);

				const std::vector<DescriptorBindingRange>& ranges = m_bindingRanges[bufferInfo.inFlightBufferId];

				{
					std::unique_lock<std::mutex> lock(m_descriptorUpdateLock);
					for (size_t i = 0; i < ranges.size(); i++) {
						const DescriptorBindingRange& range = ranges[i];
						vkCmdBindDescriptorSets(commandBuffer, bindPoint, m_pipelineLayout, range.start, static_cast<uint32_t>(range.sets.size()), range.sets.data(), 0, nullptr);
					}
				}
				for (size_t i = 0; i < m_bindlessCache.size(); i++) {
					const BindlessSetBinding& binding = m_bindlessCache[i];
					std::unique_lock<std::mutex> lock(*binding.descriptorSetLock);
					vkCmdBindDescriptorSets(commandBuffer, bindPoint, m_pipelineLayout, binding.setId, 1u, &binding.descriptorSet, 0, nullptr);
				}
			}


			VulkanEnvironmentPipeline::VulkanEnvironmentPipeline(
				VulkanDevice* device, PipelineDescriptor* descriptor, size_t maxInFlightCommandBuffers, size_t bindPointCount, const VkPipelineBindPoint* bindPoints)
				: VulkanPipeline(device, descriptor, maxInFlightCommandBuffers), m_bindPoints(bindPoints, bindPoints + bindPointCount) {}

			void VulkanEnvironmentPipeline::Execute(const CommandBufferInfo& bufferInfo) {
				VulkanCommandBuffer* commandBuffer = dynamic_cast<VulkanCommandBuffer*>(bufferInfo.commandBuffer);
				if (commandBuffer == nullptr) {
					Device()->Log()->Fatal("VulkanEnvironmentPipeline::Execute - Unsupported command buffer!");
					return;
				}
				UpdateDescriptors(bufferInfo);
				for (size_t i = 0; i < m_bindPoints.size(); i++)
					BindDescriptors(bufferInfo, m_bindPoints[i]);
				commandBuffer->RecordBufferDependency(this);
			}
		}
	}
}
#pragma warning(default: 26812)

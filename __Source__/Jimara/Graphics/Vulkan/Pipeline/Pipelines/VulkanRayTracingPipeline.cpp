#include "VulkanRayTracingPipeline.h"
#include "VulkanShader.h"
#include "../../Memory/Buffers/VulkanCpuWriteOnlyBuffer.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			Reference<VulkanRayTracingPipeline> VulkanRayTracingPipeline::Create(VulkanDevice* device, const Descriptor& descriptor) {
				if (device == nullptr)
					return nullptr;
				const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rtProps = device->PhysicalDeviceInfo()->RTFeatures().rayTracingPipelineProps;

				// Basic objects to be created:
				BindingSetBuilder builder(device);
				VkPipeline pipeline = VK_NULL_HANDLE;
				Reference<VulkanArrayBuffer> bindingTable;
				VkStridedDeviceAddressRegionKHR rgenRegion = {};
				VkStridedDeviceAddressRegionKHR missRegion = {};
				VkStridedDeviceAddressRegionKHR hitGroupRegion = {};
				VkStridedDeviceAddressRegionKHR callableRegion = {};

				// Define "log & cleanup method:
				auto fail = [&](const auto&... message) {
					bindingTable = nullptr;
					if (pipeline != VK_NULL_HANDLE) {
						vkDestroyPipeline(*device, pipeline, device->AllocationCallbacks());
						pipeline = VK_NULL_HANDLE;
					}
					device->Log()->Error("VulkanRayTracingPipeline::Create - ", message...);
					return nullptr;
				};


				// Validate some stuff:
				{
					if (!device->PhysicalDevice()->HasFeatures(PhysicalDevice::DeviceFeatures::RAY_TRACING))
						return fail("Ray-Tracing pipeline can not be created on a device with no RT support!");
					if (descriptor.maxRecursionDepth > rtProps.maxRayRecursionDepth)
						return fail("Recursion depth restricted to ", rtProps.maxRayRecursionDepth,
							" on given device (requested ", descriptor.maxRecursionDepth, ")! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				}


				// Create and cache Vulkan shader modules:
				std::unordered_map<const SPIRV_Binary*, uint32_t> shaderIndices;
				std::vector<Reference<VulkanShader>> shaders;
				const constexpr uint32_t NO_SHADER = ~uint32_t(0u);
				auto getShader = [&](const SPIRV_Binary* binary) {
					if (binary == nullptr)
						return NO_SHADER;

					// Try find existing:
					{
						auto it = shaderIndices.find(binary);
						if (it != shaderIndices.end())
							return it->second;
					}

					// Create new:
					{
						const Reference<VulkanShader> shader = VulkanShader::Create(device, binary);
						if (shader == nullptr)
							return NO_SHADER;
						builder.IncludeShaderBindings(binary);
						shaderIndices[binary] = static_cast<uint32_t>(shaders.size());
						shaders.push_back(shader);
					}

					return static_cast<uint32_t>(shaders.size() - 1u);
				};


				// Create and cache pipeline stage create infos:
				union StageKey {
					struct {
						uint32_t shaderIndex;
						uint32_t stage;
					};
					uint64_t key;
				};
				static_assert(sizeof(StageKey) == sizeof(StageKey::key));
				std::unordered_map<uint64_t, uint32_t> stageIndices;
				std::vector<VkPipelineShaderStageCreateInfo> stageCreateInfos;
				auto getStage = [&](const SPIRV_Binary* binary, VkShaderStageFlagBits stage) {
					// Get shader index:
					if (binary == nullptr)
						return VK_SHADER_UNUSED_KHR;
					const uint32_t shaderIndex = getShader(binary);
					if (shaderIndex == NO_SHADER) {
						return VK_SHADER_UNUSED_KHR;
					}

					StageKey stageKey = {};
					stageKey.shaderIndex = shaderIndex;
					stageKey.stage = static_cast<uint32_t>(stage);

					// Try finding existing record:
					{
						const auto it = stageIndices.find(stageKey.key);
						if (it != stageIndices.end())
							return it->second;
					}

					// Create new record:
					VkPipelineShaderStageCreateInfo stageInfo = {};
					{
						stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
						stageInfo.pNext = nullptr;
						stageInfo.flags = 0u;
						stageInfo.stage = stage;
						stageInfo.module = *shaders[shaderIndex];
						stageInfo.pName = "main";
						stageInfo.pSpecializationInfo = nullptr;
					}
					const uint32_t rv = static_cast<uint32_t>(stageCreateInfos.size());
					stageCreateInfos.push_back(stageInfo);
					stageIndices[stageKey.key] = rv;
					return rv;
				};


				// Shader groups:
				std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;
				auto emptyGroupDesc = [&]() {
					VkRayTracingShaderGroupCreateInfoKHR groupInfo = {};
					{
						groupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
						groupInfo.pNext = nullptr;
						groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
						groupInfo.generalShader = VK_SHADER_UNUSED_KHR;
						groupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
						groupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
						groupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
						groupInfo.pShaderGroupCaptureReplayHandle = nullptr;
					}
					return groupInfo;
				};


				// Add primary raygen shader:
				const uint32_t raygenShaderIndex = static_cast<uint32_t>(descriptor.bindingTable.size());
				{
					VkRayTracingShaderGroupCreateInfoKHR groupInfo = emptyGroupDesc();
					groupInfo.generalShader = getStage(descriptor.raygenShader, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
					if (groupInfo.generalShader == VK_SHADER_UNUSED_KHR)
						return fail("Failed to set raygen shader! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					else shaderGroups.push_back(groupInfo);
				}
				const constexpr uint32_t raygenShaderCount = 1u;


				// Add miss-shaders:
				const uint32_t firstMissShader = static_cast<uint32_t>(descriptor.bindingTable.size());
				for (size_t i = 0u; i < descriptor.missShaders.size(); i++) {
					VkRayTracingShaderGroupCreateInfoKHR groupInfo = emptyGroupDesc();
					groupInfo.generalShader = getStage(descriptor.missShaders[i], VK_SHADER_STAGE_MISS_BIT_KHR);
					if (groupInfo.generalShader == VK_SHADER_UNUSED_KHR)
						return fail("Failed to set miss shader ", i, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					else shaderGroups.push_back(groupInfo);
				}
				const uint32_t missShaderCount = static_cast<uint32_t>(descriptor.bindingTable.size() - firstMissShader);


				// Create shader groups for binding table:
				const uint32_t firstHitGroup = static_cast<uint32_t>(descriptor.bindingTable.size());
				for (size_t i = 0u; i < descriptor.bindingTable.size(); i++) {
					const ShaderGroup& group = descriptor.bindingTable[i];

					bool loadFailed = false;
					auto getStageIndex = [&](const SPIRV_Binary* binary, VkShaderStageFlagBits stage) {
						if (binary == nullptr)
							return VK_SHADER_UNUSED_KHR;
						const uint32_t stageIndex = getStage(binary, stage);
						loadFailed |= (stageIndex == VK_SHADER_UNUSED_KHR);
						return stageIndex;
					};

					VkRayTracingShaderGroupCreateInfoKHR groupInfo = emptyGroupDesc();
					{
						groupInfo.type = (group.geometryType == GeometryType::TRIANGLES)
							? VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR
							: VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
						groupInfo.closestHitShader = getStageIndex(group.closestHit, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
						groupInfo.anyHitShader = getStageIndex(group.anyHit, VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
						groupInfo.intersectionShader = getStageIndex(group.intersection, VK_SHADER_STAGE_INTERSECTION_BIT_KHR);
					}

					if (loadFailed)
						return fail("Failed to create shader module! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					else shaderGroups.push_back(groupInfo);
				}
				const uint32_t hitGroupCount = static_cast<uint32_t>(descriptor.bindingTable.size() - firstHitGroup);


				// Add callable shaders:
				const uint32_t firstCallableShader = static_cast<uint32_t>(descriptor.bindingTable.size());
				for (size_t i = 0u; i < descriptor.callableShaders.size(); i++) {
					VkRayTracingShaderGroupCreateInfoKHR groupInfo = emptyGroupDesc();
					groupInfo.generalShader = getStage(descriptor.callableShaders[i], VK_SHADER_STAGE_CALLABLE_BIT_KHR);
					if (groupInfo.generalShader == VK_SHADER_UNUSED_KHR)
						return fail("Failed to set callable shader ", i, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					else shaderGroups.push_back(groupInfo);
				}
				const uint32_t callableShaderCount = static_cast<uint32_t>(descriptor.bindingTable.size() - firstCallableShader);


				// Build pipeline layout:
				if (!builder.Finish())
					return fail("Failed to build pipeline layout! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				// Create Pipeline:
				{
					VkRayTracingPipelineCreateInfoKHR createInfo = {};
					{
						createInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
						createInfo.pNext = nullptr;
						createInfo.flags = 0u;
						createInfo.stageCount = static_cast<uint32_t>(stageCreateInfos.size());
						createInfo.pStages = stageCreateInfos.data();
						createInfo.groupCount = static_cast<uint32_t>(shaderGroups.size());
						createInfo.pGroups = shaderGroups.data();
						createInfo.maxPipelineRayRecursionDepth = descriptor.maxRecursionDepth;
						createInfo.pLibraryInfo = nullptr;
						createInfo.pLibraryInterface = nullptr;
						createInfo.pDynamicState = nullptr;
						createInfo.layout = builder.PipelineLayout();
						createInfo.basePipelineHandle = VK_NULL_HANDLE;
						createInfo.basePipelineIndex = -1;
					}
					if (device->RT().CreateRayTracingPipelinesKHR(
						*device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1u, &createInfo, device->AllocationCallbacks(), &pipeline) != VK_SUCCESS)
						return fail("Failed to create Ray-Tracing pipeline! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				}


				// Define SBT regions:
				{
					const auto alignUp = [](VkDeviceSize size, VkDeviceSize alignment) {
						return alignment * ((size + (alignment - 1u)) / alignment);
					};
					const VkDeviceSize handleSize = alignUp(rtProps.shaderGroupHandleSize, rtProps.shaderGroupHandleAlignment);

					rgenRegion.size = alignUp(handleSize, rtProps.shaderGroupBaseAlignment);
					rgenRegion.stride = rgenRegion.size;

					missRegion.size = alignUp(missShaderCount * handleSize, rtProps.shaderGroupBaseAlignment);
					missRegion.stride = handleSize;

					hitGroupRegion.size = alignUp(hitGroupCount * handleSize, rtProps.shaderGroupBaseAlignment);
					hitGroupRegion.stride = handleSize;

					callableRegion.size = alignUp(callableShaderCount * handleSize, rtProps.shaderGroupBaseAlignment);
					callableRegion.stride = handleSize;
				}

				// Create SBT buffer:
				{
					// Load data:
					const uint32_t handleCount = (1u + missShaderCount + hitGroupCount + callableShaderCount);
					std::vector<uint8_t> handleData;
					static_assert(sizeof(uint8_t) == 1u);
					handleData.resize(rtProps.shaderGroupHandleSize * handleCount);
					if (device->RT().GetRayTracingShaderGroupHandlesKHR(
						*device, pipeline, 0u, handleCount, handleData.size(), handleData.data()) != VK_SUCCESS)
						return fail("Failed to get shader group handles! [File: ", __FILE__, "; Line: ", __LINE__, "]");

					// Allocate SBT:
					bindingTable = Object::Instantiate<VulkanCpuWriteOnlyBuffer>(device, 1u,
						static_cast<size_t>(rgenRegion.size + missRegion.size + hitGroupRegion.size, callableRegion.size),
						VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
						VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
						VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR);
					if (bindingTable == nullptr)
						return fail("Failed to allocate buffer for the binding table! [File: ", __FILE__, "; Line: ", __LINE__, "]");

					// Fill data:
					uint8_t* stbData = reinterpret_cast<uint8_t*>(bindingTable->Map());
					if (stbData == nullptr)
						return fail("Failed to map STB buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");

					uint8_t* handlePtr = handleData.data();
					auto writeRegion = [&](VkStridedDeviceAddressRegionKHR& region, size_t count, const VkStridedDeviceAddressRegionKHR* prevRegion = nullptr) {
						region.deviceAddress = (prevRegion == nullptr) 
							? bindingTable->VulkanDeviceAddress() 
							: (prevRegion->deviceAddress + prevRegion->size);
						uint8_t* stbDataPtr = stbData + (region.deviceAddress - rgenRegion.deviceAddress);
						for (size_t i = 0u; i < count; i++) {
							std::memcpy(stbDataPtr, handlePtr, rtProps.shaderGroupHandleSize);
							handlePtr += rtProps.shaderGroupHandleSize;
							stbDataPtr += region.stride;
						}
					};
					writeRegion(rgenRegion, 1u);
					writeRegion(missRegion, missShaderCount, &rgenRegion);
					writeRegion(hitGroupRegion, hitGroupCount, &missRegion);
					writeRegion(callableRegion, callableShaderCount, &hitGroupRegion);

					bindingTable->Unmap(true);
				}

				const Reference<VulkanRayTracingPipeline> result = new VulkanRayTracingPipeline(
					std::move(builder), pipeline, bindingTable, rgenRegion, missRegion, hitGroupRegion, callableRegion);
				result->ReleaseRef();
				return result;
			}


			VulkanRayTracingPipeline::VulkanRayTracingPipeline(BindingSetBuilder&& builder,
				VkPipeline pipeline,
				ArrayBuffer* bindingTable,
				const VkStridedDeviceAddressRegionKHR& rgenRegion,
				const VkStridedDeviceAddressRegionKHR& missRegion,
				const VkStridedDeviceAddressRegionKHR& hitGroupRegion,
				const VkStridedDeviceAddressRegionKHR& callableRegion)
				: VulkanPipeline(std::move(builder))
				, m_pipeline(pipeline)
				, m_bindingTable(bindingTable)
				, m_rgenRegion(rgenRegion)
				, m_missRegion(missRegion)
				, m_hitGroupRegion(hitGroupRegion)
				, m_callableRegion(callableRegion) {
				assert(m_pipeline != VK_NULL_HANDLE);
				assert(m_bindingTable != nullptr);
			}

			VulkanRayTracingPipeline::~VulkanRayTracingPipeline() {
				if (m_pipeline != VK_NULL_HANDLE)
					vkDestroyPipeline(*Device(), m_pipeline, Device()->AllocationCallbacks());
			}

			void VulkanRayTracingPipeline::TraceRays(CommandBuffer* commandBuffer, const Size3& kernelSize) {
				VulkanCommandBuffer* vulkanCommandBuffer = dynamic_cast<VulkanCommandBuffer*>(commandBuffer);
				if (vulkanCommandBuffer == nullptr) {
					Device()->Log()->Error("VulkanComputePipeline::Execute - Incompatible command buffer!");
					return;
				}

				if (kernelSize.x <= 0u || kernelSize.y <= 0u || kernelSize.z <= 0u) 
					return;

				VkMemoryBarrier barrier = {};
				{
					barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
					barrier.pNext = nullptr;
					barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
					barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				}
				vkCmdPipelineBarrier(*vulkanCommandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
					VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
					VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
					0, 1, &barrier, 0, nullptr, 0, nullptr);

				vkCmdBindPipeline(*vulkanCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_pipeline);
				Device()->RT().CmdTraceRaysKHR(*vulkanCommandBuffer,
					&m_rgenRegion,
					&m_missRegion,
					&m_hitGroupRegion,
					&m_callableRegion,
					kernelSize.x, kernelSize.y, kernelSize.z);

				vulkanCommandBuffer->RecordBufferDependency(this);
			}
		}
	}
}

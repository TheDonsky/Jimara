#include "SceneLightGrid.h"
#include "../../Algorithms/SegmentTree/SegmentTreeGenerationKernel.h"
#include "../../../GraphicsSimulation/CombinedGraphicsSimulationKernel.h"


namespace Jimara {
#pragma warning(disable: 4250)
	struct SceneLightGrid::Helpers {
		struct SimulationTaskSettings {
			alignas(16) Size3 startVoxel = {};
			alignas(4) uint32_t taskThreadCount = 0u;
			alignas(16) Size3 voxelCount = {};
			alignas(4) uint32_t lightIndex = 0u;
		};
		static_assert(sizeof(SimulationTaskSettings) == 32u);

		class SharedInstance : public virtual SceneLightGrid, public virtual ObjectCache<Reference<const Object>>::StoredObject {
		private:
			const Reference<SceneContext> m_context;
			const Reference<const ViewportLightSet> m_lightSet;
			
			// Execute() should only occure once per frame and inside render jobs:
			std::atomic<uint32_t> m_canExecute = 1u;
			std::mutex m_updateLock;

			// m_localLigtIds and m_localLightBoundaries contains bounded lights and their boundaries, while m_globalLightIds stores indices of global lights:
			std::vector<uint32_t> m_localLigtIds;
			std::vector<AABB> m_localLightBoundaries;
			std::vector<uint32_t> m_globalLightIds;

			// Grid settings:
			struct GridSettings {
				alignas(16) Vector3 gridOrigin = Vector3(0.0f);
				alignas(16) Vector3 voxelSize = Vector3(1.0f);
				alignas(16) Size3 voxelGroupCount = Size3(0u);
				alignas(16) Size3 voxelGroupSize = Size3(16u);
				alignas(4) uint32_t globalLightCount = 0u;
			};
			GridSettings m_gridSettings;
			Size3 m_maxVoxelGroups = Size3(64);
			Vector3 m_targetVoxelCountPerLight = Vector3(2u);

			// 'Top-Level' voxel group buffer:
			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_voxelGroupBuffer = 
				Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
			Stacktor<Graphics::ArrayBufferReference<uint32_t>, 4u> m_voxelGroupStagingBuffers;

			// Bucket buffer:
			struct BucketRange {
				alignas(4) uint32_t start = 0u;
				alignas(4) uint32_t count = 0u;
			};
			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_bucketBuffer = 
				Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();

			// Bucket content buffer:
			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_bucketContentBuffer = 
				Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();



			inline void OnFlushed() {
				m_canExecute = 1u;
			}

			template<typename... Message>
			inline bool Fail(const Message&... message) {
				m_context->Log()->Error(message...);
				return false;
			}

			inline void UpdateLightBoundaries() {
				m_localLightBoundaries.clear();
				m_localLigtIds.clear();
				m_globalLightIds.clear();
				const ViewportLightSet::Reader lights(m_lightSet);
				for (size_t i = 0u; i < lights.LightCount(); i++) {
					const LightDescriptor::ViewportData* const lightData = lights.LightData(i);
					if (lightData == nullptr) continue;
					AABB bounds = lightData->GetLightBounds();
					if (std::isfinite(bounds.start.x) && std::isfinite(bounds.start.y) && std::isfinite(bounds.start.z) &&
						std::isfinite(bounds.end.x) && std::isfinite(bounds.end.y) && std::isfinite(bounds.end.z)) {
						if (bounds.start.x > bounds.end.x) 
							std::swap(bounds.start.x, bounds.end.x);
						if (bounds.start.y > bounds.end.y) 
							std::swap(bounds.start.y, bounds.end.y);
						if (bounds.start.z > bounds.end.z) 
							std::swap(bounds.start.z, bounds.end.z);
						m_localLigtIds.push_back(static_cast<uint32_t>(i));
						m_localLightBoundaries.push_back(bounds);
					}
					else m_globalLightIds.push_back(static_cast<uint32_t>(i));
				}
				m_gridSettings.globalLightCount = static_cast<uint32_t>(m_globalLightIds.size());
			}

			inline void UpdateGridSettings() {
				if (m_localLightBoundaries.size() <= 0u) {
					m_gridSettings.gridOrigin = Vector3(0.0f);
					m_gridSettings.voxelGroupCount = Size3(0u);
					return;
				}

				const AABB* ptr = m_localLightBoundaries.data();
				const AABB* end = ptr + m_localLightBoundaries.size();

				AABB combinedBounds = *ptr;
				Vector3 avergeSize = ptr->end - ptr->start;
				size_t count = 1u;
				ptr++;

				while (ptr < end) {
					const AABB bounds = *ptr;
					ptr++;

					if (combinedBounds.start.x > bounds.start.x)
						combinedBounds.start.x = bounds.start.x;
					if (combinedBounds.start.y > bounds.start.y)
						combinedBounds.start.y = bounds.start.y;
					if (combinedBounds.start.z > bounds.start.z)
						combinedBounds.start.z = bounds.start.z;

					if (combinedBounds.end.x < bounds.end.x)
						combinedBounds.end.x = bounds.end.x;
					if (combinedBounds.end.y < bounds.end.y)
						combinedBounds.end.y = bounds.end.y;
					if (combinedBounds.end.z < bounds.end.z)
						combinedBounds.end.z = bounds.end.z;

					count++;
					avergeSize += (bounds.end - bounds.start - avergeSize) / static_cast<float>(count);
				}

				combinedBounds.start -= 0.1f * avergeSize;
				combinedBounds.end += 0.1f * avergeSize;
				m_gridSettings.gridOrigin = combinedBounds.start;
				const Vector3 totalSize = combinedBounds.end - combinedBounds.start;

				const Size3 maxVoxelCount = m_maxVoxelGroups * m_gridSettings.voxelGroupSize;
				const Vector3 minVoxelSize = totalSize / Vector3(maxVoxelCount);
				m_gridSettings.voxelSize = Vector3(
					Math::Max(minVoxelSize.x, avergeSize.x / m_targetVoxelCountPerLight.x),
					Math::Max(minVoxelSize.y, avergeSize.y / m_targetVoxelCountPerLight.y),
					Math::Max(minVoxelSize.z, avergeSize.z / m_targetVoxelCountPerLight.z));

				const Vector3 voxelGroupSize = m_gridSettings.voxelSize * Vector3(m_gridSettings.voxelGroupSize);
				m_gridSettings.voxelGroupCount = Size3(
					static_cast<uint32_t>(std::ceilf(totalSize.x / voxelGroupSize.x)),
					static_cast<uint32_t>(std::ceilf(totalSize.y / voxelGroupSize.y)),
					static_cast<uint32_t>(std::ceilf(totalSize.z / voxelGroupSize.z)));
			}

			inline bool CalculateGridGroupRanges(const Graphics::InFlightBufferInfo& buffer) {
				// Allocate buffer:
				const size_t gridElemCount = size_t(m_gridSettings.voxelGroupCount.x) * m_gridSettings.voxelGroupCount.y * m_gridSettings.voxelGroupCount.z;
				if (m_voxelGroupBuffer->BoundObject() == nullptr || m_voxelGroupBuffer->BoundObject()->ObjectCount() < gridElemCount) {
					m_voxelGroupBuffer->BoundObject() = m_context->Graphics()->Device()->CreateArrayBuffer<uint32_t>(
						Math::Max(size_t(m_voxelGroupBuffer->BoundObject() == nullptr ? 0u : m_voxelGroupBuffer->BoundObject()->ObjectCount()), gridElemCount),
						Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY);
					if (m_voxelGroupBuffer->BoundObject() == nullptr)
						return Fail("SceneLightGrid::Helpers::SharedInstance::CalculateGridGroupRanges - ",
							"Failed to allocate Voxel group buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				};

				// Allocate staging buffer:
				if (m_voxelGroupStagingBuffers.Size() <= buffer.inFlightBufferId)
					m_voxelGroupStagingBuffers.Resize(buffer.inFlightBufferId + 1u);
				Graphics::ArrayBufferReference<uint32_t>& stagingBuffer = m_voxelGroupStagingBuffers[buffer];
				if (stagingBuffer == nullptr || stagingBuffer->ObjectCount() < gridElemCount) {
					stagingBuffer = m_context->Graphics()->Device()->CreateArrayBuffer<uint32_t>(
						m_voxelGroupBuffer->BoundObject()->ObjectCount(), Graphics::Buffer::CPUAccess::CPU_READ_WRITE);
					if (stagingBuffer == nullptr)
						return Fail("SceneLightGrid::Helpers::SharedInstance::CalculateGridGroupRanges - ",
							"Failed to allocate staging buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				}

				// Fill bucket data:
				uint32_t bucketElemCount = 0u;
				{
					uint32_t* const data = stagingBuffer.Map();
					const constexpr uint32_t noData = ~uint32_t(0u);
					
					// Cleanup:
					{
						uint32_t* ptr = data;
						uint32_t* end = data + gridElemCount;
						while (ptr < end) {
							(*ptr) = noData;
							ptr++;
						}
					}

					// Fill required buckets:
					{
						const uint32_t perBucketElemCount = m_gridSettings.voxelGroupSize.x * m_gridSettings.voxelGroupSize.y * m_gridSettings.voxelGroupSize.z;
						const AABB* ptr = m_localLightBoundaries.data();
						const AABB* end = ptr + m_localLightBoundaries.size();
						const Vector3 invBucketSize = 1.0f / (m_gridSettings.voxelSize * Vector3(m_gridSettings.voxelGroupSize));
						while (ptr < end) {
							auto toBucketSpace = [&](const Vector3& point) {
								return (point - m_gridSettings.gridOrigin) * invBucketSize;
							};
							const AABB localBounds = AABB(toBucketSpace(ptr->start), toBucketSpace(ptr->end));
							ptr++;

							const Size3 firstBucket = Size3(localBounds.start);
							const Size3 lastBucket = Size3(localBounds.end);
							if (firstBucket.x >= m_gridSettings.voxelGroupCount.x ||
								firstBucket.y >= m_gridSettings.voxelGroupCount.y ||
								firstBucket.z >= m_gridSettings.voxelGroupCount.z ||
								lastBucket.x >= m_gridSettings.voxelGroupCount.x ||
								lastBucket.y >= m_gridSettings.voxelGroupCount.y ||
								lastBucket.z >= m_gridSettings.voxelGroupCount.z)
								return Fail("SceneLightGrid::Helpers::SharedInstance::CalculateGridGroupRanges - ",
									"Internal error: bucket index out of range! [File: ", __FILE__, "; Line: ", __LINE__, "]");

							for (uint32_t x = firstBucket.x; x <= lastBucket.x; x++)
								for (uint32_t y = firstBucket.y; y <= lastBucket.y; y++)
									for (uint32_t z = firstBucket.z; z <= lastBucket.z; z++) {
										uint32_t& bucket = data[m_gridSettings.voxelGroupCount.x * (m_gridSettings.voxelGroupCount.y * z + y) + x];
										if (bucket != noData) continue;
										bucket = bucketElemCount;
										bucketElemCount += perBucketElemCount;
									}
						}
					}

					stagingBuffer->Unmap(true);
					m_bucketContentBuffer->BoundObject()->Copy(buffer, stagingBuffer, sizeof(uint32_t) * gridElemCount);
				}

				// (Re)Allocate bucket buffer:
				if (m_bucketBuffer->BoundObject() == nullptr || m_bucketBuffer->BoundObject()->ObjectCount() < bucketElemCount) {
					m_bucketBuffer->BoundObject() = m_context->Graphics()->Device()->CreateArrayBuffer<BucketRange>(
						Math::Max(size_t(m_bucketBuffer->BoundObject() == nullptr ? 0u : m_bucketBuffer->BoundObject()->ObjectCount()), gridElemCount),
						Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY);
					if (m_bucketBuffer->BoundObject() == nullptr)
						return Fail("SceneLightGrid::Helpers::SharedInstance::CalculateGridGroupRanges - ",
							"Failed to allocate bucket buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				}

				return true;
			}

			inline bool ComputePerVoxelLightCounts(const Graphics::InFlightBufferInfo& buffer) {
				// __TODO__: Implement this crap!
				return Fail("SceneLightGrid::Helpers::SharedInstance::ComputePerVoxelLightCounts - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			inline bool ComputePerVoxelLightSegmentTree(const Graphics::InFlightBufferInfo& buffer) {
				// __TODO__: Implement this crap!
				return Fail("SceneLightGrid::Helpers::SharedInstance::ComputePerVoxelLightSegmentTree - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			inline bool ComputePerVoxelIndexRanges(const Graphics::InFlightBufferInfo& buffer) {
				// __TODO__: Implement this crap!
				return Fail("SceneLightGrid::Helpers::SharedInstance::ComputePerVoxelIndexRanges - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			inline bool ComputePerVoxelLightIndices(const Graphics::InFlightBufferInfo& buffer) {
				// __TODO__: Implement this crap!
				return Fail("SceneLightGrid::Helpers::SharedInstance::ComputePerVoxelLightIndices - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

		public:
			inline SharedInstance(SceneContext* context, const ViewportLightSet* lightSet)
				: m_context(context)
				, m_lightSet(lightSet) {
				assert(m_context != nullptr);
				assert(m_lightSet != nullptr);
				m_context->Graphics()->OnGraphicsSynch() += Callback(&SharedInstance::OnFlushed, this);
			}

			inline virtual ~SharedInstance() {
				m_context->Graphics()->OnGraphicsSynch() -= Callback(&SharedInstance::OnFlushed, this);
			}

		protected:
			inline virtual void Execute()override {
				std::unique_lock<std::mutex> lock(m_updateLock);
				if (m_canExecute.load() <= 0u) return;
				m_canExecute = 0u;
				
				UpdateLightBoundaries();
				UpdateGridSettings();

				auto cleanState = [&]() {
					m_gridSettings.voxelGroupCount = Size3(0u);
					m_gridSettings.globalLightCount = 0u;
				};

				const Graphics::InFlightBufferInfo buffer = m_context->Graphics()->GetWorkerThreadCommandBuffer();
				if (!CalculateGridGroupRanges(buffer)) return cleanState();
				if (!ComputePerVoxelLightCounts(buffer)) return cleanState();
				if (!ComputePerVoxelLightSegmentTree(buffer)) return cleanState();
				if (!ComputePerVoxelIndexRanges(buffer)) return cleanState();
				if (!ComputePerVoxelLightIndices(buffer)) return cleanState();
			}

			inline virtual void CollectDependencies(Callback<JobSystem::Job*>)override {}
		};
#pragma warning(default: 4250)

		class InstanceCache : public virtual ObjectCache<Reference<const Object>> {
		public:
			static Reference<SceneLightGrid> GetFor(const ViewportLightSet* lightSet, SceneContext* context) {
				static InstanceCache cache;
				return cache.GetCachedOrCreate(lightSet, false, [&]() -> Reference<SharedInstance> {
					auto fail = [&](const auto&... message) {
						context->Log()->Error("SceneLightGrid::Helpers::InstanceCache::GetFor - ", message...);
						return nullptr;
					};

					// Create/get shared binding pool and shader set:
					const Reference<Graphics::BindingPool> bindingPool = context->Graphics()->Device()->CreateBindingPool(
						context->Graphics()->Configuration().MaxInFlightCommandBufferCount());
					if (bindingPool == nullptr)
						return fail("Failed to create binding pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					const Reference<Graphics::ShaderSet> shaderSet = context->Graphics()->Configuration().ShaderLoader()->LoadShaderSet("");
					if (shaderSet == nullptr)
						return fail("Failed to get shader set! [File: ", __FILE__, "; Line: ", __LINE__, "]");

					// Create bindings:
					Graphics::BindingSet::Descriptor bindingSetDescriptor = {};
					// __TODO__: Create bindings and fill in search functions...

					// Create kernel and input for SceneLightGrid_ZeroOutVoxelLightCounts:
					const Graphics::ShaderClass voxelLightCountClearShaderClass(
						"Jimara/Environment/Rendering/SceneObjects/Lights/SceneLightGrid_ZeroOutVoxelLightCounts");
					const Reference<Graphics::SPIRV_Binary> voxelLightCountClearShader =
						shaderSet->GetShaderModule(&voxelLightCountClearShaderClass, Graphics::PipelineStage::COMPUTE);
					if (voxelLightCountClearShader == nullptr)
						return fail("Failed to load voxelLightCountClearShader! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					const Reference<Graphics::ComputePipeline> voxelLightCountClearKernel = 
						context->Graphics()->Device()->GetComputePipeline(voxelLightCountClearShader);
					if (voxelLightCountClearKernel == nullptr)
						return fail("Failed to get/create compute pipeline for voxelLightCountClearShader! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					bindingSetDescriptor.pipeline = voxelLightCountClearKernel;
					const Reference<Graphics::BindingSet> voxelLightCountClearBindingSet = bindingPool->AllocateBindingSet(bindingSetDescriptor);
					if (voxelLightCountClearBindingSet == nullptr)
						return fail("Failed to allocate binding set for voxelLightCountClearKernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");


					// Create combined kernel for SceneLightGrid_ComputeVoxelLightCounts:
					static const Graphics::ShaderClass voxelLightCounterShader(
						"Jimara/Environment/Rendering/SceneObjects/Lights/SceneLightGrid_ComputeVoxelLightCounts");
					const Reference<CombinedGraphicsSimulationKernel<SimulationTaskSettings>> voxelLightCounter =
						CombinedGraphicsSimulationKernel<SimulationTaskSettings>::Create(context, &voxelLightCounterShader, bindingSetDescriptor.find);
					if (voxelLightCounter == nullptr)
						return fail("Failed to create combined simulation kernel for voxelLightCounter! [File: ", __FILE__, "; Line: ", __LINE__, "]");


					// Create segment tree generation kernel:
					const Reference<SegmentTreeGenerationKernel> segmentTreeGenerator = SegmentTreeGenerationKernel::CreateUintSumKernel(
						context->Graphics()->Device(), context->Graphics()->Configuration().ShaderLoader(),
						context->Graphics()->Configuration().MaxInFlightCommandBufferCount());
					if (segmentTreeGenerator == nullptr)
						return fail("Failed to create segment tree generator kernel for voxelLightCounter! [File: ", __FILE__, "; Line: ", __LINE__, "]");


					// Create kernel for SceneLightGrid_ComputeVoxelIndexRanges:
					static const Graphics::ShaderClass voxelIndexRangeCalculatorShader(
						"Jimara/Environment/Rendering/SceneObjects/Lights/SceneLightGrid_ComputeVoxelIndexRanges");
					const Reference<Graphics::SPIRV_Binary> voxelLightRangeCalculatorShaderModule = 
						shaderSet->GetShaderModule(&voxelIndexRangeCalculatorShader, Graphics::PipelineStage::COMPUTE);
					if (voxelLightRangeCalculatorShaderModule == nullptr)
						return fail("Failed to load voxelIndexRangeCalculatorShader! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					const Reference<Graphics::ComputePipeline> voxelIndexRangeCalculatorKernel =
						context->Graphics()->Device()->GetComputePipeline(voxelLightRangeCalculatorShaderModule);
					if (voxelIndexRangeCalculatorKernel == nullptr)
						return fail("Failed to get/create compute pipeline for voxelIndexRangeCalculatorShader! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					bindingSetDescriptor.pipeline = voxelIndexRangeCalculatorKernel;
					const Reference<Graphics::BindingSet> voxelIndexRangeCalculatorBindingSet = bindingPool->AllocateBindingSet(bindingSetDescriptor);
					if (voxelIndexRangeCalculatorBindingSet == nullptr)
						return fail("Failed to allocate binding set for for voxelIndexRangeCalculatorShader! [File: ", __FILE__, "; Line: ", __LINE__, "]");


					// Create combined kernel for SceneLightGrid_ComputeVoxelLightIndices:
					static const Graphics::ShaderClass voxelLightIndexFillShader(
						"Jimara/Environment/Rendering/SceneObjects/Lights/SceneLightGrid_ComputeVoxelLightIndices");
					const Reference<CombinedGraphicsSimulationKernel<SimulationTaskSettings>> voxelLightIndexFiller =
						CombinedGraphicsSimulationKernel<SimulationTaskSettings>::Create(context, &voxelLightIndexFillShader, bindingSetDescriptor.find);
					if (voxelLightIndexFiller == nullptr)
						return fail("Failed to create combined simulation kernel for voxelLightIndexFiller! [File: ", __FILE__, "; Line: ", __LINE__, "]");

					return Object::Instantiate<SharedInstance>(context, lightSet);
					});
			}
		};
	};



	SceneLightGrid::SceneLightGrid() {}

	SceneLightGrid::~SceneLightGrid() {}

	Reference<SceneLightGrid> SceneLightGrid::GetFor(SceneContext* context) {
		if (context == nullptr) return nullptr;
		const Reference<const ViewportLightSet> lightSet = ViewportLightSet::For(context);
		if (lightSet == nullptr) {
			context->Log()->Error("SceneLightGrid::GetFor - Failed to get ViewportLightSet for given context! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}
		return Helpers::InstanceCache::GetFor(lightSet, context);
	}

	Reference<SceneLightGrid> SceneLightGrid::GetFor(const ViewportDescriptor* viewport) {
		if (viewport == nullptr) return nullptr;
		const Reference<const ViewportLightSet> lightSet = ViewportLightSet::For(viewport);
		if (lightSet == nullptr) {
			viewport->Context()->Log()->Error("SceneLightGrid::GetFor - Failed to get ViewportLightSet for given viewport! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}
		return Helpers::InstanceCache::GetFor(lightSet, viewport->Context());
	}

	Graphics::BindingSet::BindingSearchFunctions SceneLightGrid::BindingDescriptor()const {
		Graphics::BindingSet::BindingSearchFunctions search = {};
		// __TODO__: Implement this crap!

		return search;
	}
}

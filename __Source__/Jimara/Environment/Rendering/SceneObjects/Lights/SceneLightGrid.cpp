#include "SceneLightGrid.h"
#include <cmath>
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

		struct GridSettings {
			alignas(16) Vector3 gridOrigin = Vector3(0.0f);
			alignas(16) Vector3 voxelSize = Vector3(1.0f);
			alignas(16) Size3 voxelGroupCount = Size3(0u);
			alignas(16) Size3 voxelGroupSize = Size3(16u);
			alignas(4) uint32_t globalLightCount = 0u;
		};

		struct BucketRange {
			alignas(4) uint32_t start = 0u;
			alignas(4) uint32_t count = 0u;
		};

		struct VoxelRangeSettings {
			alignas(4) uint32_t voxelCount = 0u;
			alignas(4) uint32_t globalLightIndexCount = 0u;
		};

		static const constexpr uint32_t BLOCK_SIZE = 256u;

		class UpdateJob : public virtual JobSystem::Job {
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
			GridSettings m_gridSettings;
			Size3 m_maxVoxelGroups = Size3(64);
			Vector3 m_targetVoxelCountPerLight = Vector3(2u);
			size_t m_activeVoxelCount = 0u;

			// Constant Bindings:
			const Graphics::BufferReference<GridSettings> m_gridSettingsBuffer;
			Reference<const Graphics::ResourceBinding<Graphics::Buffer>> m_gridSettingsBufferBinding;
			const Graphics::BufferReference<VoxelRangeSettings> m_voxelCountBuffer;

			// Voxel buffers:
			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_voxelGroupBuffer;
			Stacktor<Graphics::ArrayBufferReference<uint32_t>, 4u> m_voxelGroupStagingBuffers;
			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_voxelBuffer;
			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_segmentTreeBuffer;
			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_voxelContentBuffer;
			Stacktor<Graphics::ArrayBufferReference<uint32_t>, 4u> m_globalLightIndexBuffers;

			// Kernels:
			const Reference<Graphics::ComputePipeline> m_zeroVoxelLightCountsPipeline;
			const Reference<Graphics::BindingSet> m_zeroVoxelLightCountsBindings;
			std::vector<SimulationTaskSettings> m_perLightTaskSettings;
			const Reference<CombinedGraphicsSimulationKernel<SimulationTaskSettings>> m_computePerVoxelLightCount;
			const Reference<SegmentTreeGenerationKernel> m_generateSegmentTree;
			const Reference<Graphics::ComputePipeline> m_computeVoxelIndexRangesPipeline;
			const Reference<Graphics::BindingSet> m_computeVoxelIndexRangesBindings;
			const Reference<CombinedGraphicsSimulationKernel<SimulationTaskSettings>> m_computeVoxelLightIndices;



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
				auto updateBuffer = [&]() {
					m_gridSettingsBuffer.Map() = m_gridSettings;
					m_gridSettingsBuffer->Unmap(true);
				};

				if (m_localLightBoundaries.size() <= 0u) {
					m_gridSettings.gridOrigin = Vector3(0.0f);
					m_gridSettings.voxelGroupCount = Size3(0u);
					updateBuffer();
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
					static_cast<uint32_t>(ceilf(totalSize.x / voxelGroupSize.x)),
					static_cast<uint32_t>(ceilf(totalSize.y / voxelGroupSize.y)),
					static_cast<uint32_t>(ceilf(totalSize.z / voxelGroupSize.z)));

				updateBuffer();
			}

			inline bool CalculateGridGroupRanges(const Graphics::InFlightBufferInfo& buffer) {
				// Allocate buffer:
				const size_t gridElemCount = size_t(m_gridSettings.voxelGroupCount.x) * m_gridSettings.voxelGroupCount.y * m_gridSettings.voxelGroupCount.z;
				if (m_voxelGroupBuffer->BoundObject() == nullptr || m_voxelGroupBuffer->BoundObject()->ObjectCount() < gridElemCount) {
					m_voxelGroupBuffer->BoundObject() = m_context->Graphics()->Device()->CreateArrayBuffer<uint32_t>(
						Math::Max(size_t(m_voxelGroupBuffer->BoundObject() == nullptr ? 0u : (m_voxelGroupBuffer->BoundObject()->ObjectCount() << 1u)), gridElemCount),
						Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY);
					if (m_voxelGroupBuffer->BoundObject() == nullptr)
						return Fail("SceneLightGrid::Helpers::UpdateJob::CalculateGridGroupRanges - ",
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
						return Fail("SceneLightGrid::Helpers::UpdateJob::CalculateGridGroupRanges - ",
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
#ifndef NDEBUG
							if (firstBucket.x >= m_gridSettings.voxelGroupCount.x ||
								firstBucket.y >= m_gridSettings.voxelGroupCount.y ||
								firstBucket.z >= m_gridSettings.voxelGroupCount.z ||
								lastBucket.x >= m_gridSettings.voxelGroupCount.x ||
								lastBucket.y >= m_gridSettings.voxelGroupCount.y ||
								lastBucket.z >= m_gridSettings.voxelGroupCount.z)
								return Fail("SceneLightGrid::Helpers::UpdateJob::CalculateGridGroupRanges - ",
									"Internal error: bucket index out of range! [File: ", __FILE__, "; Line: ", __LINE__, "]");
#endif

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
					m_voxelGroupBuffer->BoundObject()->Copy(buffer, stagingBuffer, sizeof(uint32_t) * gridElemCount);
				}

				// (Re)Allocate bucket buffer:
				if (m_voxelBuffer->BoundObject() == nullptr || m_voxelBuffer->BoundObject()->ObjectCount() < bucketElemCount) {
					m_voxelBuffer->BoundObject() = m_context->Graphics()->Device()->CreateArrayBuffer<BucketRange>(
						Math::Max(
							size_t(m_voxelBuffer->BoundObject() == nullptr ? 0u : (m_voxelBuffer->BoundObject()->ObjectCount() << 1u)), 
							size_t(bucketElemCount)),
						Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY);
					if (m_voxelBuffer->BoundObject() == nullptr)
						return Fail("SceneLightGrid::Helpers::UpdateJob::CalculateGridGroupRanges - ",
							"Failed to allocate bucket buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				}

				m_activeVoxelCount = bucketElemCount;
				VoxelRangeSettings voxelCountSettings = {};
				{
					voxelCountSettings.voxelCount = static_cast<uint32_t>(m_activeVoxelCount);
					voxelCountSettings.globalLightIndexCount = static_cast<uint32_t>(m_globalLightIds.size());
				}
				m_voxelCountBuffer.Map() = voxelCountSettings;
				m_voxelCountBuffer->Unmap(true);
				return true;
			}

			inline bool UpdateGlobalLightIndexBuffers(const Graphics::InFlightBufferInfo& buffer) {
				// (Re)Allocate global buffers:
				if (m_globalLightIndexBuffers.Size() <= buffer.inFlightBufferId)
					m_globalLightIndexBuffers.Resize(buffer.inFlightBufferId + 1u);
				Graphics::ArrayBufferReference<uint32_t>& stagingBuffer = m_globalLightIndexBuffers[buffer.inFlightBufferId];
				if (stagingBuffer == nullptr || stagingBuffer->ObjectCount() < m_globalLightIds.size()) {
					stagingBuffer = m_context->Graphics()->Device()->CreateArrayBuffer<uint32_t>(
						Math::Max(stagingBuffer == nullptr ? size_t(0u) : stagingBuffer->ObjectCount() << 1u, m_globalLightIds.size()),
						Graphics::Buffer::CPUAccess::CPU_READ_WRITE);
					if (stagingBuffer == nullptr)
						return Fail("SceneLightGrid::Helpers::UpdateJob::UpdateGlobalLightIndexBuffers - ",
							"Failed to allocate staging buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				}

				// Update global lights buffer:
				std::memcpy(stagingBuffer->Map(), m_globalLightIds.data(), sizeof(uint32_t) * m_globalLightIds.size());
				stagingBuffer->Unmap(true);
				return true;
			}

			inline bool ComputePerVoxelIndexRanges(const Graphics::InFlightBufferInfo& buffer) {
				// Calculate per-light task settings:
				size_t contentBufferSize = m_globalLightIds.size();
				{
					m_perLightTaskSettings.clear();
					const AABB* ptr = m_localLightBoundaries.data();
					const AABB* end = ptr + m_localLightBoundaries.size();
					const uint32_t* indexPtr = m_localLigtIds.data();
					const Vector3 invBucketSize = 1.0f / m_gridSettings.voxelSize;
#ifndef DEBUG
					const Size3 voxelCount = m_gridSettings.voxelGroupCount * m_gridSettings.voxelGroupSize;
#endif
					while (ptr < end) {
						auto toVoxelSpace = [&](const Vector3& point) {
							return (point - m_gridSettings.gridOrigin) * invBucketSize;
						};
						const AABB localBounds = AABB(toVoxelSpace(ptr->start), toVoxelSpace(ptr->end));
						const uint32_t lightIndex = (*indexPtr);
						ptr++;
						indexPtr++;

						const Size3 firstVoxel = Size3(localBounds.start);
						const Size3 lastVoxel = Size3(localBounds.end);
#ifndef NDEBUG
						if (firstVoxel.x >= voxelCount.x ||
							firstVoxel.y >= voxelCount.y ||
							firstVoxel.z >= voxelCount.z ||
							lastVoxel.x >= voxelCount.x ||
							lastVoxel.y >= voxelCount.y ||
							lastVoxel.z >= voxelCount.z)
							return Fail("SceneLightGrid::Helpers::UpdateJob::ComputePerVoxelIndexRanges - ",
								"Internal error: bucket index out of range! [File: ", __FILE__, "; Line: ", __LINE__, "]");
#endif

						SimulationTaskSettings settings = {};
						{
							settings.startVoxel = firstVoxel;
							settings.voxelCount = (lastVoxel - firstVoxel) + 1u;
							settings.taskThreadCount = (settings.voxelCount.x * settings.voxelCount.y * settings.voxelCount.z);
							settings.lightIndex = lightIndex;
						}
						m_perLightTaskSettings.push_back(settings);
						contentBufferSize += settings.taskThreadCount;
					}
				}

				// (Re)Allocate segment tree buffer:
				const size_t segmentTreeElemCount = SegmentTreeGenerationKernel::SegmentTreeBufferSize(m_activeVoxelCount);
				if (m_segmentTreeBuffer->BoundObject() == nullptr || m_segmentTreeBuffer->BoundObject()->ObjectCount() < segmentTreeElemCount) {
					m_segmentTreeBuffer->BoundObject() = m_context->Graphics()->Device()->CreateArrayBuffer<BucketRange>(
						Math::Max(
							size_t(m_segmentTreeBuffer->BoundObject() == nullptr ? 0u : (m_segmentTreeBuffer->BoundObject()->ObjectCount() << 1u)),
							size_t(segmentTreeElemCount)),
						Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY);
					if (m_segmentTreeBuffer->BoundObject() == nullptr)
						return Fail("SceneLightGrid::Helpers::UpdateJob::ComputePerVoxelIndexRanges - ",
							"Failed to allocate segment tree buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				}

				// (Re)Allocate content buffer:
				if (m_voxelContentBuffer->BoundObject() == nullptr || m_voxelContentBuffer->BoundObject()->ObjectCount() < contentBufferSize) {
					m_voxelContentBuffer->BoundObject() = m_context->Graphics()->Device()->CreateArrayBuffer<BucketRange>(
						Math::Max(
							size_t(m_voxelContentBuffer->BoundObject() == nullptr ? 0u : (m_voxelContentBuffer->BoundObject()->ObjectCount() << 1u)),
							size_t(contentBufferSize)),
						Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY);
					if (m_voxelContentBuffer->BoundObject() == nullptr)
						return Fail("SceneLightGrid::Helpers::UpdateJob::ComputePerVoxelIndexRanges - ",
							"Failed to allocate voxel content buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				}


				// Fill in global light indices:
				m_voxelContentBuffer->BoundObject()->Copy(buffer, m_globalLightIndexBuffers[buffer], sizeof(uint32_t) * m_globalLightIds.size());

				// Execute count cleanup kernel:
				const Size3 numBlocks = Size3((m_activeVoxelCount + BLOCK_SIZE - 1u) / BLOCK_SIZE, 1u, 1u);
				m_zeroVoxelLightCountsBindings->Update(buffer);
				m_zeroVoxelLightCountsBindings->Bind(buffer);
				m_zeroVoxelLightCountsPipeline->Dispatch(buffer, numBlocks);

				// Execute unified counter kernel:
				m_computePerVoxelLightCount->Execute(buffer, m_perLightTaskSettings.data(), m_perLightTaskSettings.size());

				// Build segment tree:
				m_generateSegmentTree->Execute(buffer, m_segmentTreeBuffer->BoundObject(), m_activeVoxelCount, true);

				// Execute range generator kernel:
				m_computeVoxelIndexRangesBindings->Update(buffer);
				m_computeVoxelIndexRangesBindings->Bind(buffer);
				m_computeVoxelIndexRangesPipeline->Dispatch(buffer, numBlocks);

				// Execute unified fill kernel:
				m_computeVoxelLightIndices->Execute(buffer, m_perLightTaskSettings.data(), m_perLightTaskSettings.size());

				return true;
			}

		public:
			inline UpdateJob(
				SceneContext* context, const ViewportLightSet* lightSet,
				const Graphics::BufferReference<GridSettings>& gridSettingsBuffer,
				const Graphics::BufferReference<VoxelRangeSettings>& voxelCountBuffer,
				Graphics::ResourceBinding<Graphics::ArrayBuffer>* voxelGroupBuffer,
				Graphics::ResourceBinding<Graphics::ArrayBuffer>* voxelBuffer,
				Graphics::ResourceBinding<Graphics::ArrayBuffer>* segmentTreeBuffer,
				Graphics::ResourceBinding<Graphics::ArrayBuffer>* voxelContentBuffer,
				Graphics::ComputePipeline* zeroVoxelLightCountsPipeline,
				Graphics::BindingSet* zeroVoxelLightCountsBindings,
				CombinedGraphicsSimulationKernel<SimulationTaskSettings>* computePerVoxelLightCount,
				SegmentTreeGenerationKernel* generateSegmentTree,
				Graphics::ComputePipeline* computeVoxelIndexRangesPipeline,
				Graphics::BindingSet* computeVoxelIndexRangesBindings,
				CombinedGraphicsSimulationKernel<SimulationTaskSettings>* computeVoxelLightIndices)
				: m_context(context)
				, m_lightSet(lightSet)
				, m_gridSettingsBuffer(gridSettingsBuffer)
				, m_voxelCountBuffer(voxelCountBuffer)
				, m_voxelGroupBuffer(voxelGroupBuffer)
				, m_voxelBuffer(voxelBuffer)
				, m_segmentTreeBuffer(segmentTreeBuffer)
				, m_voxelContentBuffer(voxelContentBuffer)
				, m_zeroVoxelLightCountsPipeline(zeroVoxelLightCountsPipeline)
				, m_zeroVoxelLightCountsBindings(zeroVoxelLightCountsBindings)
				, m_computePerVoxelLightCount(computePerVoxelLightCount)
				, m_generateSegmentTree(generateSegmentTree)
				, m_computeVoxelIndexRangesPipeline(computeVoxelIndexRangesPipeline)
				, m_computeVoxelIndexRangesBindings(computeVoxelIndexRangesBindings)
				, m_computeVoxelLightIndices(computeVoxelLightIndices) {
				assert(m_context != nullptr);
				assert(m_lightSet != nullptr);
				assert(m_gridSettingsBuffer != nullptr);
				assert(m_voxelCountBuffer != nullptr);
				assert(m_voxelGroupBuffer != nullptr);
				assert(m_voxelBuffer != nullptr);
				assert(m_segmentTreeBuffer != nullptr);
				assert(m_voxelContentBuffer != nullptr);
				assert(m_zeroVoxelLightCountsPipeline != nullptr);
				assert(m_zeroVoxelLightCountsBindings != nullptr);
				assert(m_computePerVoxelLightCount != nullptr);
				assert(m_generateSegmentTree != nullptr);
				assert(m_computeVoxelIndexRangesPipeline != nullptr);
				assert(m_computeVoxelIndexRangesBindings != nullptr);
				assert(m_computeVoxelLightIndices != nullptr);
				m_gridSettingsBufferBinding = Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(m_gridSettingsBuffer);
				m_context->Graphics()->OnGraphicsSynch() += Callback(&UpdateJob::OnFlushed, this);
			}

			inline virtual ~UpdateJob() {
				m_context->Graphics()->OnGraphicsSynch() -= Callback(&UpdateJob::OnFlushed, this);
			}

			inline Reference<const Graphics::ResourceBinding<Graphics::Buffer>> FindConstantBuffer(const Graphics::BindingSet::BindingDescriptor& desc)const {
				if (desc.name == "SceneLightGrid_settingsBuffer") 
					return m_gridSettingsBufferBinding;
				else return nullptr;
			};
			inline Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> FindStructuredBuffer(const Graphics::BindingSet::BindingDescriptor& desc)const {
				if (desc.name == "SceneLightGrid_voxelGroupBuffer")
					return m_voxelGroupBuffer;
				else if (desc.name == "SceneLightGrid_voxelBuffer")
					return m_voxelBuffer;
				else if (desc.name == "SceneLightGrid_lightIndexBuffer")
					return m_voxelContentBuffer;
				else return nullptr;
			};

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
				if (!UpdateGlobalLightIndexBuffers(buffer)) return cleanState();
				if (!ComputePerVoxelIndexRanges(buffer)) return cleanState();
			}

			inline virtual void CollectDependencies(Callback<JobSystem::Job*>)override {}
		};

		class UpdateEnforcerJob : public virtual JobSystem::Job {
		private:
			const Reference<Helpers::UpdateJob> m_updateJob;
		public:
			inline UpdateEnforcerJob(Helpers::UpdateJob* job) : m_updateJob(job) {}
			inline virtual ~UpdateEnforcerJob() {}
		protected:
			inline virtual void Execute()override {}
			inline virtual void CollectDependencies(Callback<JobSystem::Job*> report)override { report(m_updateJob); }
		};

		struct SharedInstance : public virtual SceneLightGrid, public virtual ObjectCache<Reference<const Object>>::StoredObject {
			const Reference<SceneContext> m_context;
			const Reference<Helpers::UpdateJob> m_updateJob;
			const Reference<UpdateEnforcerJob> m_updateEnforcerJob;
			
			inline SharedInstance(SceneContext* context, Helpers::UpdateJob* updateJob) 
				: m_context(context), m_updateJob(updateJob)
				, m_updateEnforcerJob(Object::Instantiate<UpdateEnforcerJob>(updateJob)) {
				assert(m_context != nullptr);
				assert(m_updateJob != nullptr);
				m_context->Graphics()->RenderJobs().Add(m_updateEnforcerJob);
			}

			virtual ~SharedInstance() {
				m_context->Graphics()->RenderJobs().Remove(m_updateEnforcerJob);
			}
		};

#pragma warning(default: 4250)

		class InstanceCache : public virtual ObjectCache<Reference<const Object>> {
		public:
			static Reference<SceneLightGrid> GetFor(const ViewportLightSet* lightSet, SceneContext* context) {
				static InstanceCache cache;
				return cache.GetCachedOrCreate(lightSet, [&]() -> Reference<SharedInstance> {
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
					
					const Graphics::BufferReference<GridSettings> gridSettingsBuffer = context->Graphics()->Device()->CreateConstantBuffer<GridSettings>();
					if (gridSettingsBuffer == nullptr)
						return fail("Failed to create grid settings buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					const Reference<const Graphics::ResourceBinding<Graphics::Buffer>> gridSettingsBinding = 
						Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(gridSettingsBuffer);
					const Graphics::BufferReference<VoxelRangeSettings> liveVoxelCountBuffer = context->Graphics()->Device()->CreateConstantBuffer<VoxelRangeSettings>();
					if (liveVoxelCountBuffer == nullptr)
						return fail("Failed to create live voxel count buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					const Reference<const Graphics::ResourceBinding<Graphics::Buffer>> liveVoxelCountBinding =
						Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(liveVoxelCountBuffer);
					auto findConstantBuffer = [&](const auto& info) -> const Graphics::ResourceBinding<Graphics::Buffer>* {
						if (info.name == "gridSettings") return gridSettingsBinding;
						else if (info.name == "voxelRangeSettings") return liveVoxelCountBinding;
						else return nullptr;
					};
					bindingSetDescriptor.find.constantBuffer = &findConstantBuffer;

					const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> segmentTreeBuffer =
						Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
					const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> voxelGroupBuffer =
						Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
					const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> voxelRangeBuffer =
						Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
					const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> voxelContentBuffer =
						Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
					auto findStructuredBuffer = [&](const auto& info) -> const Graphics::ResourceBinding<Graphics::ArrayBuffer>* {
						if (info.name == "counts") return segmentTreeBuffer;
						else if (info.name == "voxelGroups") return voxelGroupBuffer;
						else if (info.name == "voxels") return voxelRangeBuffer;
						else if (info.name == "voxelContent") return voxelContentBuffer;
						else return nullptr;
					};
					bindingSetDescriptor.find.structuredBuffer = &findStructuredBuffer;


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

					const Reference<Helpers::UpdateJob> updateJob = Object::Instantiate<Helpers::UpdateJob>(
						context, lightSet,
						gridSettingsBuffer,
						liveVoxelCountBuffer,
						voxelGroupBuffer,
						voxelRangeBuffer,
						segmentTreeBuffer,
						voxelContentBuffer,
						voxelLightCountClearKernel,
						voxelLightCountClearBindingSet,
						voxelLightCounter,
						segmentTreeGenerator,
						voxelIndexRangeCalculatorKernel,
						voxelIndexRangeCalculatorBindingSet,
						voxelLightIndexFiller);
					return Object::Instantiate<SharedInstance>(context, updateJob);
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
		const Helpers::UpdateJob* self = dynamic_cast<const Helpers::SharedInstance*>(this)->m_updateJob;
		search.constantBuffer = Function(&Helpers::UpdateJob::FindConstantBuffer, self);
		search.structuredBuffer = Function(&Helpers::UpdateJob::FindStructuredBuffer, self);
		return search;
	}

	JobSystem::Job* SceneLightGrid::UpdateJob()const {
		return dynamic_cast<const Helpers::SharedInstance*>(this)->m_updateJob;
	}
}

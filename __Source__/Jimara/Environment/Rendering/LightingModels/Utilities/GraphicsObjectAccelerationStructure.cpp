#include "GraphicsObjectAccelerationStructure.h"
#include "SceneAccelerationStructures.h"
#include "../../../GraphicsSimulation/GraphicsSimulation.h"


namespace std {
	size_t hash<Jimara::GraphicsObjectAccelerationStructure::Descriptor>::operator()(
		const Jimara::GraphicsObjectAccelerationStructure::Descriptor& descriptor)const {
		return Jimara::MergeHashes(
			std::hash<decltype(descriptor.descriptorSet)>()(descriptor.descriptorSet),
			std::hash<decltype(descriptor.frustrumDescriptor)>()(descriptor.frustrumDescriptor),
			std::hash<decltype(descriptor.layers)>()(descriptor.layers),
			std::hash<decltype(descriptor.flags)>()(descriptor.flags));
	}
}

namespace Jimara {
	bool GraphicsObjectAccelerationStructure::Descriptor::operator==(const Descriptor& other)const {
		return
			descriptorSet == other.descriptorSet &&
			frustrumDescriptor == other.frustrumDescriptor &&
			layers == other.layers &&
			flags == other.flags;
	}

	bool GraphicsObjectAccelerationStructure::Descriptor::operator<(const Descriptor& other)const {
#define GraphicsObjectAccelerationStructure_Descriptor_Compare_Field(field) \
	if (field < other.field) return true; else if (field > other.field) return false
		GraphicsObjectAccelerationStructure_Descriptor_Compare_Field(descriptorSet);
		GraphicsObjectAccelerationStructure_Descriptor_Compare_Field(frustrumDescriptor);
		GraphicsObjectAccelerationStructure_Descriptor_Compare_Field(layers);
		GraphicsObjectAccelerationStructure_Descriptor_Compare_Field(flags);
#undef GraphicsObjectAccelerationStructure_Descriptor_Compare_Field
		return false;
	}

	struct GraphicsObjectAccelerationStructure::Helpers {
		struct GraphicsObjectData {
			Reference<GraphicsObjectDescriptor> graphicsObject;
			mutable Reference<const GraphicsObjectDescriptor::ViewportData> viewportData;

			inline GraphicsObjectData(GraphicsObjectDescriptor* desc = nullptr) : graphicsObject(desc) {}

			inline ~GraphicsObjectData() {}
		};

		class GraphicsObjectSet : public virtual Object {
		private:
			std::recursive_mutex m_descriptorLock;
			Descriptor m_desc;

			std::shared_mutex m_dataLock;
			ObjectSet<GraphicsObjectDescriptor, GraphicsObjectData> m_graphicsObjectData;

			inline void OnGraphicsObjectsAdded(GraphicsObjectDescriptor* const* descriptors, size_t count) {
				std::unique_lock<decltype(m_dataLock)> lock(m_dataLock);
				GraphicsObjectDescriptor* const* const end = descriptors + count;
				for (GraphicsObjectDescriptor* const* ptr = descriptors; ptr < end; ptr++) {
					// Obtain descriptor:
					GraphicsObjectDescriptor* desc = *ptr;
					if (desc == nullptr)
						continue;

					// Check flags:
					if (!m_desc.layers[desc->Layer()])
						continue;

					// __TODO__: Filter by flags..

					// Obtain viewport-data:
					const Reference<const GraphicsObjectDescriptor::ViewportData> data = desc->GetViewportData(m_desc.frustrumDescriptor);
					if (data == nullptr)
						continue;

					// We only build triangle-based BLAS-es:
					if (data->GeometryType() != Graphics::GraphicsPipeline::IndexType::TRIANGLE)
						continue;

					// Add to graphics object data:
					m_graphicsObjectData.Add(&desc, 1u, [&](const GraphicsObjectData* inserted, size_t insertedCount) {
						if (insertedCount <= 0u)
							return;
						assert(insertedCount == 1u);
						inserted->viewportData = data;
						});
				}
			}

			inline void OnGraphicsObjectsRemoved(GraphicsObjectDescriptor* const* descriptors, size_t count) {
				std::unique_lock<decltype(m_dataLock)> lock(m_dataLock);
				GraphicsObjectDescriptor* const* const end = descriptors + count;
				for (GraphicsObjectDescriptor* const* ptr = descriptors; ptr < end; ptr++)
					m_graphicsObjectData.Remove(*ptr);
			}

		public:
			inline GraphicsObjectSet() {}

			inline virtual ~GraphicsObjectSet() {
				Clear();
			}

			inline bool Initialize(const Descriptor& desc) {
				std::unique_lock<decltype(m_descriptorLock)> lock(m_descriptorLock);
				if (m_desc == desc)
					return true;
				Clear();

				if (desc.descriptorSet == nullptr)
					return true;

				desc.descriptorSet->OnAdded() += Callback<GraphicsObjectDescriptor* const*, size_t>(&GraphicsObjectSet::OnGraphicsObjectsAdded, this);
				desc.descriptorSet->OnRemoved() += Callback<GraphicsObjectDescriptor* const*, size_t>(&GraphicsObjectSet::OnGraphicsObjectsRemoved, this);
				{
					std::vector<GraphicsObjectDescriptor*> currentDescriptors;
					desc.descriptorSet->GetAll([&](GraphicsObjectDescriptor* desc) {
						if (desc == nullptr)
							return;
						desc->AddRef();
						currentDescriptors.push_back(desc);
						});
					OnGraphicsObjectsAdded(currentDescriptors.data(), currentDescriptors.size());
					for (size_t i = 0u; i < currentDescriptors.size(); i++)
						currentDescriptors[i]->ReleaseRef();
					currentDescriptors.clear();
				}

				{
					std::unique_lock<decltype(m_dataLock)> lock(m_dataLock);
					m_desc = desc;
				}
				return true;
			}

			inline void Clear() {
				std::unique_lock<decltype(m_descriptorLock)> lock(m_descriptorLock);
				if (m_desc.descriptorSet != nullptr) {
					m_desc.descriptorSet->OnAdded() -= Callback<GraphicsObjectDescriptor* const*, size_t>(&GraphicsObjectSet::OnGraphicsObjectsAdded, this);
					m_desc.descriptorSet->OnRemoved() -= Callback<GraphicsObjectDescriptor* const*, size_t>(&GraphicsObjectSet::OnGraphicsObjectsRemoved, this);
				}
				{
					std::unique_lock<decltype(m_dataLock)> lock(m_dataLock);
					m_graphicsObjectData.Clear();
					m_desc.descriptorSet = nullptr;
					m_desc = {};
				}
			}

			class Reader final {
			private:
				std::shared_lock<decltype(GraphicsObjectSet::m_dataLock)> m_lock;
				const Reference<GraphicsObjectSet> m_set;

			public:
				inline Reader(GraphicsObjectSet* set) : m_lock(set->m_dataLock), m_set(set) {}
				inline ~Reader() {}

				inline size_t Size()const { return m_set->m_graphicsObjectData.Size(); }
				inline const GraphicsObjectData& operator[](size_t index)const { return m_set->m_graphicsObjectData[index]; }
				inline SceneContext* Context()const { return m_set->m_desc.descriptorSet->Context(); }
			};
		};

		struct GraphicsObjectBlasRange {
			size_t firstBlas = 0u;
			size_t blasCount = 0u;
		};

		struct GraphicsObjectGeometry {
			Reference<const GraphicsObjectDescriptor> graphicsObject;
			Reference<const GraphicsObjectDescriptor::ViewportData> viewportData;
			GraphicsObjectDescriptor::GeometryDescriptor geometry = {};

			SceneAccelerationStructures::BlasDesc blasDesc = {};
			GraphicsObjectBlasRange oldBlasRange;
			GraphicsObjectBlasRange blasRange;

			Graphics::IndirectDrawBufferReference indirectDrawCommands;
			Reference<Graphics::ArrayBuffer> instanceTransforms;
			size_t instanceTransformOffset = 0u;
			size_t instanceTransformSride = 0u;
			size_t drawCount = 0u;
		};

		class BlasCollector : public virtual JobSystem::Job {
		private:
			const Reference<GraphicsSimulation::JobDependencies> m_simulationJobs;
			const Reference<SceneAccelerationStructures> m_blasProvider;
			const Reference<GraphicsObjectSet> m_objectSet;
			std::shared_mutex m_stateLock;
			uint64_t m_lastUpdateFrame = 0u;
			std::vector<GraphicsObjectGeometry> m_objectGeometry;
			std::vector<Reference<SceneAccelerationStructures::Blas>> m_oldBlasInstances;
			std::vector<Reference<SceneAccelerationStructures::Blas>> m_blasInstances;
			Reference<SceneContext> m_context;
			
		public:
			inline BlasCollector(
				GraphicsSimulation::JobDependencies* simulationJobs,
				SceneAccelerationStructures* blasProvider,
				GraphicsObjectSet* objectSet)
				: m_simulationJobs(simulationJobs)
				, m_blasProvider(blasProvider)
				, m_objectSet(objectSet) {
				assert(m_simulationJobs != nullptr);
				assert(m_blasProvider != nullptr);
				assert(m_objectSet != nullptr);
				GraphicsObjectSet::Reader reader(objectSet);
				m_context = reader.Context();
				assert(m_context != nullptr);
				m_lastUpdateFrame = m_context->FrameIndex() - 1u;
			}
			inline virtual ~BlasCollector() {}

		protected:
			// __TODO__: This class is supposed to read GraphicsObjectSet each frame and fill a corresponding list of Blas instances.
			// Should preferrably run before scene acceleration structures get built.
			// It should also wait for the graphics simulation to finish.

			virtual void Execute() {
				GraphicsObjectSet::Reader objectSet(m_objectSet);
				std::unique_lock<decltype(m_stateLock)> lock(m_stateLock);

				// Avoid double-execution caused by whatever reasons...
				{
					if (objectSet.Context() != nullptr)
						m_context = objectSet.Context();
					assert(m_context != nullptr);
					const uint64_t frame = m_context->FrameIndex();
					if (frame == m_lastUpdateFrame)
						return;
					else m_lastUpdateFrame = frame;
				}

				// Resize if we have missing entries (we check this to avoid prematurely loosing reference to resources that might be relocated):
				if (m_objectGeometry.size() < objectSet.Size())
					m_objectGeometry.resize(objectSet.Size());

				// Reset blas instance list:
				std::swap(m_oldBlasInstances, m_blasInstances);
				m_blasInstances.clear();

				for (size_t i = 0u; i < objectSet.Size(); i++) {
					const GraphicsObjectData& data = objectSet[i];
					GraphicsObjectGeometry& object = m_objectGeometry[i];

					// Copy graphics object info:
					object.graphicsObject = data.graphicsObject;
					object.viewportData = data.viewportData;

					// Keep old blas:
					object.oldBlasRange = object.blasRange;

					// Extract geometry:
					GraphicsObjectDescriptor::GeometryDescriptor geometry = {};
					object.viewportData->GetGeometry(geometry);
					object.geometry = geometry;
					const size_t blasCount =
						(geometry.vertexPositions.perInstanceStride <= 0u) ? 1u :
						geometry.instances.count;

					// Define blas descriptors:
					SceneAccelerationStructures::BlasDesc blasDesc = {};
					{
						blasDesc.vertexBuffer = geometry.vertexPositions.buffer;
						blasDesc.indexBuffer = geometry.indexBuffer.buffer;
						blasDesc.vertexFormat = Graphics::BottomLevelAccelerationStructure::VertexFormat::X32Y32Z32;
						blasDesc.indexFormat =
							(blasDesc.indexBuffer == nullptr || blasDesc.indexBuffer->ObjectSize() == sizeof(uint16_t))
							? Graphics::BottomLevelAccelerationStructure::IndexFormat::U16
							: Graphics::BottomLevelAccelerationStructure::IndexFormat::U32;
						blasDesc.vertexPositionOffset = geometry.vertexPositions.bufferOffset;
						blasDesc.vertexStride = geometry.vertexPositions.perVertexStride;
						blasDesc.vertexCount = geometry.vertexPositions.numEntriesPerInstance;
						blasDesc.faceCount = geometry.indexBuffer.indexCount / 3u;
						const uint32_t indexSize = (blasDesc.indexFormat == decltype(blasDesc.indexFormat)::U16) ? sizeof(uint16_t) : sizeof(uint32_t);
						blasDesc.indexOffset = geometry.indexBuffer.baseIndexOffset / indexSize;
						if ((blasDesc.indexOffset * indexSize) != geometry.indexBuffer.baseIndexOffset) {
							blasDesc.indexBuffer = nullptr;
							objectSet.Context()->Log()->Warning("GraphicsObjectAccelerationStructure::Helpers::BlasCollector::Execute - ",
								"Index buffer offset not a multiple of index stride; skipping graphics object! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						}
						blasDesc.flags = 
							((object.graphicsObject->Shader()->BlendMode() == Material::BlendMode::Opaque)
								? decltype(blasDesc.flags)::NONE
								: decltype(blasDesc.flags)::PREVENT_DUPLICATE_ANY_HIT_INVOCATIONS) |
							(((geometry.flags & decltype(geometry.flags)::VERTEX_POSITION_CONSTANT) != decltype(geometry.flags)::NONE)
								? decltype(blasDesc.flags)::NONE
								: (decltype(blasDesc.flags)::REBUILD_ON_EACH_FRAME |
									decltype(blasDesc.flags)::PREFER_FAST_BUILD |
									decltype(blasDesc.flags)::REFIT_ON_REBUILD));
						blasDesc.displacementJob = Unused<Graphics::CommandBuffer*, uint64_t>;
						blasDesc.displacementJobId = 0u;
					}

					// If blas descriptor is 'empty'/invalid, we can ignore the entry:
					if (blasDesc.vertexBuffer == nullptr ||
						blasDesc.indexBuffer == nullptr ||
						blasDesc.vertexCount <= 0u ||
						blasDesc.faceCount <= 0u) {
						object.blasRange = { 0u, 0u };
						blasDesc = {};
						object.blasDesc = blasDesc;
					}
					else {
						object.blasRange.firstBlas = m_blasInstances.size();
						
						// If we have a new blas-descriptor, we need to update the BLAS:
						if (object.blasDesc != blasDesc || object.blasRange.blasCount != blasCount) {
							object.blasDesc = blasDesc;
							object.blasRange.blasCount = 0u;
							for (size_t i = 0u; i < blasCount; i++) {
								const Reference<SceneAccelerationStructures::Blas> blas = m_blasProvider->GetBlas(blasDesc);
								blasDesc.vertexPositionOffset += geometry.vertexPositions.perInstanceStride;
								if (blas == nullptr) {
									objectSet.Context()->Log()->Error("GraphicsObjectAccelerationStructure::Helpers::BlasCollector::Execute - ",
										"Failed to create blas; skipping the instance! [File: ", __FILE__, "; Line: ", __LINE__, "]");
									continue;
								}
								m_blasInstances.push_back(blas);
								object.blasRange.blasCount++;
							}
						}

						// If blas-descriptor is kept, we can just copy the content:
						else for (size_t i = 0u; i < object.blasRange.blasCount; i++)
							m_blasInstances.push_back(m_oldBlasInstances[object.oldBlasRange.firstBlas + i]);
					}
				}

				// Resize if we have extra entries:
				if (m_objectGeometry.size() > objectSet.Size())
					m_objectGeometry.resize(objectSet.Size());
			}

			virtual void CollectDependencies(Callback<Job*> addDependency) {
				// We can run as soon as the simulation jobs are complete:
				m_simulationJobs->CollectDependencies(addDependency);
			}

		public:
			class Reader final {
			private:
				std::shared_lock<decltype(BlasCollector::m_stateLock)> m_lock;

				const Reference<BlasCollector> m_set;

			public:
				inline Reader(BlasCollector* set) : m_lock(set->m_stateLock), m_set(set) {}
				inline ~Reader() {}

				inline const GraphicsObjectGeometry* Geometry(size_t index = 0u)const { return m_set->m_objectGeometry.data() + index; }
				inline const size_t GeometryCount()const { return m_set->m_objectGeometry.size(); }
				inline size_t GetBlasses(const GraphicsObjectGeometry& geometry,
					std::vector<Reference<Graphics::BottomLevelAccelerationStructure>> blasses)const {
					if (geometry.blasRange.blasCount <= 0u)
						return 0u;

					size_t blasCount = 0u;
					auto fillBlasses = [&](
						const Reference<SceneAccelerationStructures::Blas>* const blasList,
						size_t firstBlas, size_t blasCount, auto failureHandler) {
							const Reference<SceneAccelerationStructures::Blas>* ptr = blasList + firstBlas;
							const Reference<SceneAccelerationStructures::Blas>* const end = ptr + blasCount;
							while (ptr < end) {
								const Reference<SceneAccelerationStructures::Blas>& blasRef = (*ptr);
								assert(blasRef != nullptr);
								const Reference<Graphics::BottomLevelAccelerationStructure> blas = blasRef->AcccelerationStructure();
								if (blas != nullptr) {
									blasses.push_back(blas);
									blasCount++;
								}
								else if (!failureHandler())
									break;
								ptr++;
							}
						};

					fillBlasses(m_set->m_blasInstances.data(),
						geometry.blasRange.firstBlas, geometry.blasRange.blasCount,
						[&]() {
							// Default to old blas instances if the new ones have not finished initialization to avoid flickering where possible:
							// Normally, this will never happen, but if we decide to make BLAS initialization lazier, this might serve us good.
							blasses.resize(blasses.size() - blasCount);
							blasCount = 0u;
							fillBlasses(m_set->m_oldBlasInstances.data(),
								geometry.oldBlasRange.firstBlas, geometry.oldBlasRange.blasCount,
								[]() { return true; });
							return false;
						});

					return blasCount;
				}
				inline SceneContext* Context()const { return m_set->m_context; }
			};
		};


		struct LiveRangesSettings {
			alignas(8) uint64_t liveInstanceRangeBufferOrSegmentTreeSize = 0u;	// Bytes [0 - 8)
			alignas(4) uint32_t firstInstanceIndexOffset = 0u;					// Bytes [8 - 12)
			alignas(4) uint32_t firstInstanceIndexStride = 0u;					// Bytes [12 - 16)
			alignas(4) uint32_t instanceCountOffset =  0u;						// Bytes [16 - 20)
			alignas(4) uint32_t instanceCountStride = 0u;						// Bytes [20 - 24)

			alignas(4) uint32_t liveRangeStart = 0u;							// Bytes [24 - 28)
			alignas(4) uint32_t taskThreadCount = 0u;							// Bytes [28 - 32)
		};

		static_assert(sizeof(LiveRangesSettings) == 32u);
		static_assert(offsetof(LiveRangesSettings, liveInstanceRangeBufferOrSegmentTreeSize) == 0u);
		static_assert(offsetof(LiveRangesSettings, firstInstanceIndexOffset) == 8u);
		static_assert(offsetof(LiveRangesSettings, firstInstanceIndexStride) == 12u);
		static_assert(offsetof(LiveRangesSettings, instanceCountOffset) == 16u);
		static_assert(offsetof(LiveRangesSettings, instanceCountStride) == 20u);

		static_assert(offsetof(LiveRangesSettings, liveRangeStart) == 24u);
		static_assert(offsetof(LiveRangesSettings, taskThreadCount) == 28u);

		struct InstanceGeneratorSettings : LiveRangesSettings {
			alignas(8) uint64_t jm_objectTransformBuffer = 0u;			// Bytes [32 - 40)
			alignas(4) uint32_t jm_objectTransformBufferStride = 0u;	// Bytes [40 - 44)

			alignas(4) uint32_t liveInstanceRangeCount = 0u;			// Bytes [44 - 48)
		};

		static_assert(offsetof(InstanceGeneratorSettings, liveInstanceRangeBufferOrSegmentTreeSize) == 0u);
		static_assert(offsetof(InstanceGeneratorSettings, firstInstanceIndexOffset) == 8u);
		static_assert(offsetof(InstanceGeneratorSettings, firstInstanceIndexStride) == 12u);
		static_assert(offsetof(InstanceGeneratorSettings, instanceCountOffset) == 16u);
		static_assert(offsetof(InstanceGeneratorSettings, instanceCountStride) == 20u);

		static_assert(offsetof(InstanceGeneratorSettings, liveRangeStart) == 24u);
		static_assert(offsetof(InstanceGeneratorSettings, taskThreadCount) == 28u);

		static_assert(offsetof(InstanceGeneratorSettings, jm_objectTransformBuffer) == 32u);
		static_assert(offsetof(InstanceGeneratorSettings, jm_objectTransformBufferStride) == 40u);

		static_assert(offsetof(InstanceGeneratorSettings, liveInstanceRangeCount) == 44u);
		static_assert(offsetof(InstanceGeneratorSettings, jm_objectTransformBuffer) == sizeof(LiveRangesSettings));
		static_assert(sizeof(InstanceGeneratorSettings) == 48u);

		class TlasBuilder : public virtual JobSystem::Job {
		private:
			const Reference<SceneAccelerationStructures> m_blasProvider;
			const Reference<BlasCollector> m_blasCollector;
			// __TODO__: This class is supposed to run after the BLAS instances are updated and should build the TLAS from them.

			std::shared_mutex m_stateLock;
			uint64_t m_lastUpdateFrame = 0u;


		public:
			inline TlasBuilder(SceneAccelerationStructures* blasProvider, BlasCollector* blasCollector)
				: m_blasProvider(blasProvider), m_blasCollector(blasCollector) {
				assert(m_blasProvider != nullptr);
				assert(m_blasCollector != nullptr);
				m_lastUpdateFrame = BlasCollector::Reader(m_blasCollector).Context()->FrameIndex() - 1u;
			}
			inline virtual ~TlasBuilder() {}

		protected:
			virtual void Execute() {
				BlasCollector::Reader reader(m_blasCollector);
				std::unique_lock<decltype(m_stateLock)> lock(m_stateLock);

				// Avoid double-execution caused by whatever reasons...
				{
					const uint64_t frame = reader.Context()->FrameIndex();
					if (frame == m_lastUpdateFrame)
						return;
					else m_lastUpdateFrame = frame;
				}

				// Kernel settings:
				std::vector<LiveRangesSettings> liveRangeCalculationSettings;
				std::vector<InstanceGeneratorSettings> instanceGenraratorSettings;
				uint32_t segmentTreeSize = 0u;
				uint32_t totalInstanceCount = 0u;

				// Iterate over geometries:
				const GraphicsObjectGeometry* const geometries = reader.Geometry();
				const size_t geometryCount = reader.GeometryCount();
				std::vector<Reference<Graphics::BottomLevelAccelerationStructure>> blasses;
				for (size_t objectId = 0u; objectId < geometryCount; objectId++) {
					const GraphicsObjectGeometry& geometry = geometries[objectId];
					const size_t firstBlas = blasses.size();
					const size_t blasCount = reader.GetBlasses(geometry, blasses);

					// __TODO__: Implement this crap! [EI BUILD TLASS instance buffer and the TLAS itself]!!
					
					// Fill live ranges:
					LiveRangesSettings liveRanges = {};
					{
						liveRanges.liveInstanceRangeBufferOrSegmentTreeSize =
							(geometry.geometry.instances.liveInstanceRangeBuffer != nullptr)
							? geometry.geometry.instances.liveInstanceRangeBuffer->DeviceAddress() : 0u;
						liveRanges.firstInstanceIndexOffset = geometry.geometry.instances.firstInstanceIndexOffset;
						liveRanges.firstInstanceIndexStride = geometry.geometry.instances.firstInstanceIndexStride;
						liveRanges.instanceCountOffset = geometry.geometry.instances.instanceCountOffset;
						liveRanges.instanceCountStride = geometry.geometry.instances.instanceCountStride;

						liveRanges.liveRangeStart = segmentTreeSize;
						liveRanges.taskThreadCount = geometry.geometry.instances.liveInstanceRangeCount;
					}

					// If we have have more than one range, we add to the range calculation tasks:
					if (liveRanges.liveInstanceRangeBufferOrSegmentTreeSize != 0u &&
						liveRanges.taskThreadCount > 1u &&
						geometry.geometry.instances.count > 0u) {
						liveRangeCalculationSettings.push_back(liveRanges);
						segmentTreeSize += (geometry.geometry.instances.count + 1u);
					}
					
					// Instance generator settings:
					InstanceGeneratorSettings instances = {};
					{
						(*static_cast<LiveRangesSettings*>(&instances)) = liveRanges;

						instances.taskThreadCount = geometry.geometry.instances.count;
						instances.liveInstanceRangeCount =
							(liveRanges.liveInstanceRangeBufferOrSegmentTreeSize != 0u)
							? geometry.geometry.instances.liveInstanceRangeCount : 0u;

						instances.jm_objectTransformBuffer = (geometry.geometry.instanceTransforms.buffer != nullptr)
							? (geometry.geometry.instanceTransforms.buffer->DeviceAddress() + geometry.geometry.instanceTransforms.bufferOffset) : 0u;
						instances.jm_objectTransformBufferStride = geometry.geometry.instanceTransforms.elemStride;

						// __TODO__: Collect more information for instance generation...
					}

					// If we actually have instances, add to the tasks:
					if (instances.taskThreadCount > 0u) {
						totalInstanceCount += instances.taskThreadCount;
						instanceGenraratorSettings.push_back(instances);
					}
				}

				// Make sure to store segment tree size instead of live-instance-range-buffer when the range count exceeds 1:
				for (size_t i = 0u; i < instanceGenraratorSettings.size(); i++) {
					InstanceGeneratorSettings& settings = instanceGenraratorSettings[i];
					if (settings.liveInstanceRangeCount > 1u)
						settings.liveInstanceRangeBufferOrSegmentTreeSize = segmentTreeSize;
				}

				if (liveRangeCalculationSettings.size() > 0u) {
					// __TODO__: Obtain a transient buffer for the liveRangeMarkers.
					// __TODO__: Zero-out liveRangeMarkers.
					// __TODO__: Execute LiveRanges kernel.
					// __TODO__: Calculate segment-tree for the liveRangeMarkers.
				}

				// __TODO__: Build BLAS instances and per-instance metadata.
				// __TODO__: Build TLAS using the instances.
			}

			virtual void CollectDependencies(Callback<Job*> addDependency) {
				// We can run once blas-es are collected and built for this frame:
				m_blasProvider->CollectBuildJobs(addDependency);
				addDependency(m_blasCollector);
			}
		};

		class ScheduledJob : public virtual JobSystem::Job {
		private:
			const Reference<TlasBuilder> m_buildJob;

		public:
			inline ScheduledJob(TlasBuilder* buildJob) : m_buildJob(buildJob) {
				assert(m_buildJob != nullptr);
			}
			inline virtual ~ScheduledJob() {}

		protected:
			virtual void Execute() {}

			virtual void CollectDependencies(Callback<Job*> addDependency) {
				addDependency(m_buildJob);
			}
		};

		class Instance : public virtual GraphicsObjectAccelerationStructure {
		public:
			inline Instance() {
			}

			inline virtual ~Instance() {
				Clear();
			}

			inline bool Initialize(const Descriptor& desc) {
				std::unique_lock<decltype(m_initializationLock)> lock(m_initializationLock);
				
				// Nothing to do if descriptor is unchanged:
				if (m_desc == desc)
					return true;

				// Remove previous state:
				Clear();

				// Nothing to do, if there's no underlying descriptor set:
				if (desc.descriptorSet == nullptr || desc.descriptorSet->Context() == nullptr)
					return true;

				auto fail = [&](const auto&... message) {
					desc.descriptorSet->Context()->Log()->Error("GraphicsObjectAccelerationStructure::Helpers::Instance::Initialize - ", message...);
					Clear();
					return false;
					};

				// Create graphics object set:
				m_objectSet = Object::Instantiate<GraphicsObjectSet>();
				if (!m_objectSet->Initialize(desc)) {
					Clear();
					return false;
				}

				// Obtain simulation jobs:
				const Reference<GraphicsSimulation::JobDependencies> simulationJobs = GraphicsSimulation::JobDependencies::For(desc.descriptorSet->Context());
				if (simulationJobs == nullptr)
					return fail("Can not obtain simulation jobs!");

				// Get BLAS builder:
				const Reference<SceneAccelerationStructures> blasBuilder = SceneAccelerationStructures::Get(desc.descriptorSet->Context());
				if (blasBuilder == nullptr)
					return fail("Can not obtain SceneAccelerationStructures!");

				// Create jobs:
				m_blasCollector = Object::Instantiate<BlasCollector>(simulationJobs, blasBuilder, m_objectSet);
				m_tlasBuilder = Object::Instantiate<TlasBuilder>(blasBuilder, m_blasCollector);
				
				// Schedule update jobs:
				m_scheduledJob = Object::Instantiate<ScheduledJob>(m_tlasBuilder);
				desc.descriptorSet->Context()->Graphics()->RenderJobs().Add(m_scheduledJob);

				// Done:
				m_desc = desc;
				return true;
			}

			inline void Clear() {
				std::unique_lock<decltype(m_initializationLock)> lock(m_initializationLock);
				
				// Remove scheduled job:
				if (m_scheduledJob != nullptr) {
					assert(m_desc.descriptorSet != nullptr);
					assert(m_desc.descriptorSet->Context() != nullptr);
					m_desc.descriptorSet->Context()->Graphics()->RenderJobs().Remove(m_scheduledJob);
					m_scheduledJob = nullptr;
				}

				// Remove jobs:
				m_tlasBuilder = nullptr;
				m_blasCollector = nullptr;

				// Remove object set:
				if (m_objectSet != nullptr) {
					m_objectSet->Clear();
					m_objectSet = nullptr;
				}

				// Reset descriptor:
				m_desc.descriptorSet = nullptr;
				m_desc = {};
			}

		private:
			std::recursive_mutex m_initializationLock;
			Descriptor m_desc;
			Reference<GraphicsObjectSet> m_objectSet;
			Reference<BlasCollector> m_blasCollector;
			Reference<TlasBuilder> m_tlasBuilder;
			Reference<ScheduledJob> m_scheduledJob;
		};

#pragma warning(disable: 4250)
		class SharedInstance : public virtual Instance, public virtual ObjectCache<Descriptor>::StoredObject {
		public:
			inline SharedInstance() {}
			inline virtual ~SharedInstance() {}
		};

		class InstanceCache : public virtual ObjectCache<Descriptor> {
		public:
			static Reference<SharedInstance> GetInstance(const Descriptor& desc) {
				static InstanceCache cache;
				Reference<SharedInstance> instance = cache.GetCachedOrCreate(desc, Object::Instantiate<SharedInstance>);
				if (instance != nullptr && instance->RefCount() == 1u) // Just created.
					if (!instance->Initialize(desc))
						return nullptr;
				return instance;
			}
		};
#pragma warning(default: 4250)
	};

	Reference<GraphicsObjectAccelerationStructure> GraphicsObjectAccelerationStructure::GetFor(const Descriptor& desc) {
		if (desc.descriptorSet == nullptr)
			return nullptr;
		else if (!desc.descriptorSet->Context()
			->Graphics()->Device()->PhysicalDevice()->HasFeatures(Graphics::PhysicalDevice::DeviceFeatures::RAY_TRACING)) {
			desc.descriptorSet->Context()->Log()->Error("SceneAccelerationStructures::Get - ",
				"Graphics device does not support hardware Ray-Tracing features! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}
		else return Helpers::InstanceCache::GetInstance(desc);
	}
}

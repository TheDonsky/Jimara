#include "GraphicsObjectAccelerationStructure.h"
#include "SceneAccelerationStructures.h"
#include "../../../../Graphics/Memory/TransientBufferSet.h"
#include "../../../GraphicsSimulation/GraphicsSimulation.h"
#include "../../Algorithms/SegmentTree/SegmentTreeGenerationKernel.h"
#include "../../../GraphicsSimulation/CombinedGraphicsSimulationKernel.h"


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

				{
					std::unique_lock<decltype(m_dataLock)> lock(m_dataLock);
					m_desc = desc;
				}

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
						// __TODO__: We need refit in general, but it's a bit unsafe now...
						//blasDesc.flags &= ~decltype(blasDesc.flags)::REFIT_ON_REBUILD;
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
							// Culled instances might not [yet] have normal BLAS instances..
							const size_t safeBlasCount =
								(geometry.instances.liveInstanceRangeBuffer != nullptr) ? blasCount : Math::Min(blasCount,
									(object.blasDesc == blasDesc)
									? Math::Max(object.blasRange.blasCount, size_t(geometry.instances.liveInstanceEntryCount))
									: geometry.instances.liveInstanceEntryCount);

							// We can have some shared blasses:
							const size_t sharedBlasCount =
								(object.blasDesc != blasDesc) ? 0u :
								Math::Min(safeBlasCount, object.blasRange.blasCount);

							// Update desc:
							object.blasDesc = blasDesc;
							object.blasRange.blasCount = 0u;

							// Reuse some blasses:
							for (size_t i = 0u; i < sharedBlasCount; i++)
								m_blasInstances.push_back(m_oldBlasInstances[object.oldBlasRange.firstBlas + i]);
							blasDesc.vertexPositionOffset += static_cast<uint32_t>(geometry.vertexPositions.perInstanceStride * sharedBlasCount);
							object.blasRange.blasCount = sharedBlasCount;

							// Update blasses:
							for (size_t i = sharedBlasCount; i < safeBlasCount; i++) {
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
					std::vector<Reference<Graphics::BottomLevelAccelerationStructure>>& blasses)const {
					if (geometry.blasRange.blasCount <= 0u)
						return 0u;

					size_t fetchedBlasCount = 0u;
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
									fetchedBlasCount++;
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
							blasses.resize(blasses.size() - fetchedBlasCount);
							fetchedBlasCount = 0u;
							fillBlasses(m_set->m_oldBlasInstances.data(),
								geometry.oldBlasRange.firstBlas, geometry.oldBlasRange.blasCount,
								[]() { return true; });
							return false;
						});

					return fetchedBlasCount;
				}
				inline SceneContext* Context()const { return m_set->m_context; }
			};
		};


		struct LiveRangesSettings {
			alignas(8) uint64_t liveInstanceRangeBuffer = 0u;	// Bytes [0 - 8)
			alignas(4) uint32_t firstInstanceIndexOffset = 0u;	// Bytes [8 - 12)
			alignas(4) uint32_t firstInstanceIndexStride = 0u;	// Bytes [12 - 16)
			alignas(4) uint32_t instanceCountOffset =  0u;		// Bytes [16 - 20)
			alignas(4) uint32_t instanceCountStride = 0u;		// Bytes [20 - 24)

			alignas(4) uint32_t liveRangeStart = 0u;			// Bytes [24 - 28)
			alignas(4) uint32_t taskThreadCount = 0u;			// Bytes [28 - 32)
		};

		static_assert(sizeof(LiveRangesSettings) == 32u);
		static_assert(offsetof(LiveRangesSettings, liveInstanceRangeBuffer) == 0u);
		static_assert(offsetof(LiveRangesSettings, firstInstanceIndexOffset) == 8u);
		static_assert(offsetof(LiveRangesSettings, firstInstanceIndexStride) == 12u);
		static_assert(offsetof(LiveRangesSettings, instanceCountOffset) == 16u);
		static_assert(offsetof(LiveRangesSettings, instanceCountStride) == 20u);

		static_assert(offsetof(LiveRangesSettings, liveRangeStart) == 24u);
		static_assert(offsetof(LiveRangesSettings, taskThreadCount) == 28u);

		using LiveRangeKernel = CombinedGraphicsSimulationKernel<LiveRangesSettings>;

		struct InstanceGeneratorSettings {
			alignas(8) uint64_t liveInstanceRangeBufferOrSegmentTreeSize = 0u;			// Bytes [0 - 8)
			alignas(4) uint32_t firstInstanceIndexOffset = 0u;							// Bytes [8 - 12)
			alignas(4) uint32_t instanceCountOffset = 0u;								// Bytes [12 - 16)

			alignas(4) uint32_t liveRangeStart = 0u;									// Bytes [16 - 24)
			alignas(4) uint32_t taskThreadCount = 0u;									// Bytes [24 - 28)

			alignas(8) uint64_t jm_objectTransformBuffer = 0u;							// Bytes [28 - 36)
			alignas(4) uint32_t jm_objectTransformBufferStride = 0u;					// Bytes [36 - 40)

			alignas(4) uint32_t liveInstanceRangeCount = 0u;							// Bytes [40 - 44)

			// If blasCount is 1, blasReference is direct reference, otherwise, we'll have per-instance entries and it'll be a buffer element address.
			alignas(8) uint64_t blasReference = 0u;										// Bytes [44 - 48)
			alignas(4) uint32_t blasCount = 0u;											// Bytes [48 - 52)

			// Instance custom index and visibility-mask for the BLAS instances.
			alignas(4) uint32_t instanceCustomIndex24_visibilityMask8 = 0u;				// Bytes [52 - 56)

			// SBT and instance flags for the BLAS instances.
			alignas(4) uint32_t shaderBindingTableRecordOffset24_instanceFlags8 = 0u;	// Bytes [56 - 60)

			alignas(4) uint32_t firstInstanceIndex = 0u;								// Bytes [60 - 64)
		};

		static_assert(offsetof(InstanceGeneratorSettings, liveInstanceRangeBufferOrSegmentTreeSize) == 0u);
		static_assert(offsetof(InstanceGeneratorSettings, firstInstanceIndexOffset) == 8u);
		static_assert(offsetof(InstanceGeneratorSettings, instanceCountOffset) == 12u);

		static_assert(offsetof(InstanceGeneratorSettings, liveRangeStart) == 16u);
		static_assert(offsetof(InstanceGeneratorSettings, taskThreadCount) == 20u);

		static_assert(offsetof(InstanceGeneratorSettings, jm_objectTransformBuffer) == 24u);
		static_assert(offsetof(InstanceGeneratorSettings, jm_objectTransformBufferStride) == 32u);

		static_assert(offsetof(InstanceGeneratorSettings, liveInstanceRangeCount) == 36u);

		static_assert(offsetof(InstanceGeneratorSettings, blasReference) == 40u);
		static_assert(offsetof(InstanceGeneratorSettings, blasCount) == 48u);

		static_assert(offsetof(InstanceGeneratorSettings, instanceCustomIndex24_visibilityMask8) == 52u);

		static_assert(offsetof(InstanceGeneratorSettings, shaderBindingTableRecordOffset24_instanceFlags8) == 56);

		static_assert(offsetof(InstanceGeneratorSettings, firstInstanceIndex) == 60);
		static_assert(sizeof(InstanceGeneratorSettings) == 64u);

		using InstanceGeneratorKernel = CombinedGraphicsSimulationKernel<InstanceGeneratorSettings>;
		
		struct TlasSnapshot : public virtual Object {
			std::shared_mutex lock;
			std::vector<ObjectInformation> objectInformation;
			std::vector<Reference<Graphics::BottomLevelAccelerationStructure>> blasses;
			Reference<Graphics::TopLevelAccelerationStructure> tlas;
			Matrix4 rayTransform = Math::Identity();
		};

		struct TlasSnapshots : public virtual Object {
			std::vector<Reference<TlasSnapshot>> inFlightSnapshots;
			SpinLock activeSnapshotLock;
			Reference<TlasSnapshot> activeSnapshot;
		};

		class TlasBuilder : public virtual JobSystem::Job {
		public:
			class Kernels final {
			private:
				Reference<Graphics::TransientBufferSet> transientBuffers;
				Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> liveRangeBuffer;
				Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> instanceDescriptors;
				mutable std::vector<Graphics::ArrayBufferReference<uint64_t>> blasReferenceStagingBuffer;
				Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> blasReferenceBuffer;
				Reference<LiveRangeKernel> liveRangeKernel;
				Reference<SegmentTreeGenerationKernel> segementTreeKernel;
				Reference<InstanceGeneratorKernel> instanceGeneratorKernel;

				inline Kernels() {}
				inline ~Kernels() {}

				friend class TlasBuilder;
			};

		private:
			const Reference<SceneAccelerationStructures> m_blasProvider;
			const Reference<BlasCollector> m_blasCollector;
			const Kernels m_kernels;
			const Reference<TlasSnapshots> m_tlasSnapshots;

			std::shared_mutex m_stateLock;
			uint64_t m_lastUpdateFrame = 0u;

			Reference<Graphics::TopLevelAccelerationStructure> m_tlas;
			size_t m_blasInstanceBudget = 1u;


		public:
			inline TlasBuilder(
				SceneAccelerationStructures* blasProvider, 
				BlasCollector* blasCollector, 
				const Kernels* kernels,
				TlasSnapshots* tlasSnapshots)
				: m_blasProvider(blasProvider)
				, m_blasCollector(blasCollector)
				, m_kernels(*kernels)
				, m_tlasSnapshots(tlasSnapshots) {
				assert(m_blasProvider != nullptr);
				assert(m_blasCollector != nullptr);
				assert(m_kernels.transientBuffers != nullptr);
				assert(m_kernels.liveRangeBuffer != nullptr);
				assert(m_kernels.instanceDescriptors != nullptr);
				assert(m_kernels.blasReferenceBuffer != nullptr);
				assert(m_kernels.liveRangeKernel != nullptr);
				assert(m_kernels.segementTreeKernel != nullptr);
				assert(m_kernels.instanceGeneratorKernel != nullptr);
				assert(m_tlasSnapshots != nullptr);
				BlasCollector::Reader reader(m_blasCollector);
				std::unique_lock<decltype(m_tlasSnapshots->activeSnapshotLock)> lock(m_tlasSnapshots->activeSnapshotLock);
				while (m_tlasSnapshots->inFlightSnapshots.size() < reader.Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount())
					m_tlasSnapshots->inFlightSnapshots.push_back(Object::Instantiate<TlasSnapshot>());
				m_tlasSnapshots->inFlightSnapshots.resize(reader.Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount());
				m_lastUpdateFrame = reader.Context()->FrameIndex() - 1u;
			}
			inline virtual ~TlasBuilder() {}

		protected:
			virtual void Execute() {
				BlasCollector::Reader reader(m_blasCollector);
				std::unique_lock<decltype(m_stateLock)> lock(m_stateLock);

				static const constexpr bool ENFORCE_SEGMENT_TREE_GENERATION_FOR_SINGLE_RANGE_OBJECTS = false;

				// Avoid double-execution caused by whatever reasons...
				{
					const uint64_t frame = reader.Context()->FrameIndex();
					if (frame == m_lastUpdateFrame)
						return;
					else m_lastUpdateFrame = frame;
				}
				auto fail = [&](const auto&... message) {
					reader.Context()->Log()->Error("GraphicsObjectAccelerationStructure::Helpers::TlasBuilder::Create - ", message...);
				};

				// Kernel settings:
				std::vector<LiveRangesSettings> liveRangeCalculationSettings;
				std::vector<InstanceGeneratorSettings> instanceGeneratorSettings;
				std::vector<ObjectInformation> objectInformation;
				uint32_t segmentTreeSize = 0u;
				uint32_t totalInstanceCount = 0u;
				size_t indirectBlasReferenceCount = 0u;

				// Iterate over geometries:
				const GraphicsObjectGeometry* const geometries = reader.Geometry();
				const size_t geometryCount = reader.GeometryCount();
				std::vector<Reference<Graphics::BottomLevelAccelerationStructure>> blasses;
				for (size_t objectId = 0u; objectId < geometryCount; objectId++) {
					const GraphicsObjectGeometry& geometry = geometries[objectId];
					const size_t firstBlas = blasses.size();
					const size_t blasCount = reader.GetBlasses(geometry, blasses);

					// Fill live ranges:
					LiveRangesSettings liveRanges = {};
					{
						liveRanges.liveInstanceRangeBuffer =
							(geometry.geometry.instances.liveInstanceRangeBuffer != nullptr)
							? geometry.geometry.instances.liveInstanceRangeBuffer->DeviceAddress() : 0u;
						liveRanges.firstInstanceIndexOffset = geometry.geometry.instances.firstInstanceIndexOffset;
						liveRanges.firstInstanceIndexStride = geometry.geometry.instances.firstInstanceIndexStride;
						liveRanges.instanceCountOffset = geometry.geometry.instances.instanceCountOffset;
						liveRanges.instanceCountStride = geometry.geometry.instances.instanceCountStride;

						liveRanges.liveRangeStart = segmentTreeSize;
						liveRanges.taskThreadCount = (liveRanges.liveInstanceRangeBuffer != 0u)
							? geometry.geometry.instances.liveInstanceEntryCount : 0u;
					}

					// If we have have more than one range, we add to the range calculation tasks:
					if (liveRanges.liveInstanceRangeBuffer != 0u &&
						(liveRanges.taskThreadCount > 1u || ENFORCE_SEGMENT_TREE_GENERATION_FOR_SINGLE_RANGE_OBJECTS) &&
						geometry.geometry.instances.count > 0u) {
						liveRangeCalculationSettings.push_back(liveRanges);
						segmentTreeSize += (geometry.geometry.instances.count + 2u);
					}
					
					// Instance generator settings:
					InstanceGeneratorSettings instances = {};
					{
						instances.liveInstanceRangeBufferOrSegmentTreeSize = liveRanges.liveInstanceRangeBuffer;
						instances.firstInstanceIndexOffset = liveRanges.firstInstanceIndexOffset;
						instances.instanceCountOffset = liveRanges.instanceCountOffset;

						instances.liveRangeStart = liveRanges.liveRangeStart;

						instances.taskThreadCount =
							(liveRanges.liveInstanceRangeBuffer != 0u)
							? geometry.geometry.instances.count
							: geometry.geometry.instances.liveInstanceEntryCount;
						if (geometry.geometry.vertexPositions.perInstanceStride > 0u)
							instances.taskThreadCount = Math::Min(instances.taskThreadCount, static_cast<uint32_t>(blasCount));

						instances.liveInstanceRangeCount =
							(liveRanges.liveInstanceRangeBuffer != 0u)
							? geometry.geometry.instances.liveInstanceEntryCount : 0u;
						
						instances.jm_objectTransformBuffer = (geometry.geometry.instanceTransforms.buffer != nullptr)
							? (geometry.geometry.instanceTransforms.buffer->DeviceAddress() + geometry.geometry.instanceTransforms.bufferOffset) : 0u;
						instances.jm_objectTransformBufferStride = geometry.geometry.instanceTransforms.elemStride;

						if (blasCount > 1u) {
							instances.blasReference = indirectBlasReferenceCount;
							indirectBlasReferenceCount += blasCount;
						}
						else instances.blasReference = (blasCount > 0u) ? blasses[firstBlas]->DeviceAddress() : uint64_t(0u);
						instances.blasCount = static_cast<uint32_t>(blasCount);

						union {
							struct {
								uint32_t first24 : 24;
								uint32_t last8 : 8u;
							} bits;
							uint32_t value;
						} bitfield;
						static_assert(sizeof(bitfield) == sizeof(uint32_t));

						assert(objectInformation.size() == instanceGeneratorSettings.size());
						bitfield.bits.first24 = static_cast<uint32_t>(objectInformation.size());
						bitfield.bits.last8 = 255u;
						instances.instanceCustomIndex24_visibilityMask8 = bitfield.value;

						bitfield.bits.first24 = 0u;
						const Material::MaterialFlags materialFlags = geometry.graphicsObject->Shader()->MaterialFlags();
						const bool canDiscard = ((materialFlags & Material::MaterialFlags::CanDiscard) != Material::MaterialFlags::NONE);
						const bool isOpaque = geometry.graphicsObject->Shader()->BlendMode() == Material::BlendMode::Opaque;
						bitfield.bits.last8 = static_cast<uint32_t>(
							// Opaque faces do not need any-hits:
							((isOpaque && (!canDiscard)) 
							? Graphics::AccelerationStructureInstanceDesc::Flags::FORCE_OPAQUE
							: Graphics::AccelerationStructureInstanceDesc::Flags::NONE) |
							
							//Graphics::AccelerationStructureInstanceDesc::Flags::DISABLE_BACKFACE_CULLING |
							
							// Winding order seems to be inverted.
							Graphics::AccelerationStructureInstanceDesc::Flags::FLIP_FACES);
						instances.shaderBindingTableRecordOffset24_instanceFlags8 = bitfield.value;

						instances.firstInstanceIndex = static_cast<uint32_t>(totalInstanceCount);
					}

					// If we actually have instances, add to the tasks:
					{
						// We could validate taskThreadCount > 0, but 0-count tasks will be convenient, 
						// because they'll keep the reader object lists consistent.
						static const constexpr uint32_t UINT24_MAX = (1u << 24u) - 1u;
						if (instanceGeneratorSettings.size() >= UINT24_MAX) {
							fail("Instance custom index is nor allowed to exceed ", UINT24_MAX,
								"/UINT24_MAX! Skipping the instance! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						}
						else {
							instanceGeneratorSettings.push_back(instances);

							ObjectInformation objectInfo = {};
							{
								objectInfo.graphicsObject = geometry.graphicsObject;
								objectInfo.viewportData = geometry.viewportData;
								objectInfo.geometry = geometry.geometry;
								objectInfo.firstInstanceIndex = instances.firstInstanceIndex;
								objectInfo.instanceCount = instances.taskThreadCount;
								objectInfo.firstBlas = static_cast<uint32_t>(firstBlas);
								objectInfo.blasCount = static_cast<uint32_t>(blasCount);
							}
							objectInformation.push_back(objectInfo);

							totalInstanceCount += instances.taskThreadCount;
						}
					}
				}

				// Make sure to store segment tree size instead of live-instance-range-buffer when the range count exceeds 1:
				for (size_t i = 0u; i < instanceGeneratorSettings.size(); i++) {
					InstanceGeneratorSettings& settings = instanceGeneratorSettings[i];
					if (ENFORCE_SEGMENT_TREE_GENERATION_FOR_SINGLE_RANGE_OBJECTS && settings.liveInstanceRangeCount == 1u)
						settings.liveInstanceRangeCount = 2u;
					if (settings.liveInstanceRangeCount > 1u)
						settings.liveInstanceRangeBufferOrSegmentTreeSize = segmentTreeSize;
				}

				// Obtain the buffer for the segment-tree:
				const size_t segmentTreeBufferSize = SegmentTreeGenerationKernel::SegmentTreeBufferSize(segmentTreeSize);
				{
					const size_t minRequieredCount = Math::Max(segmentTreeBufferSize, size_t(1u));
					const size_t minRequiredSize = (sizeof(int32_t) * minRequieredCount);
					if (m_kernels.liveRangeBuffer->BoundObject() == nullptr ||
						m_kernels.liveRangeBuffer->BoundObject()->Size() < minRequiredSize) {
						m_kernels.liveRangeBuffer->BoundObject() = m_kernels.transientBuffers->GetBuffer(minRequiredSize, 0u);
						if (m_kernels.liveRangeBuffer->BoundObject() == nullptr) {
							return fail("Failed to obtain transient buffer for the segment-tree! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						}
					}
				}

				// Obtain instance descriptor buffer:
				{
					const size_t minRequiredBufferSize = sizeof(Graphics::AccelerationStructureInstanceDesc) * Math::Max(totalInstanceCount, uint32_t(1u));
					if (m_kernels.instanceDescriptors->BoundObject() == nullptr ||
						m_kernels.instanceDescriptors->BoundObject()->Size() < minRequiredBufferSize) {
						size_t count = 1u;
						while (count < totalInstanceCount)
							count <<= 1u;
						m_kernels.instanceDescriptors->BoundObject() = reader.Context()->Graphics()->Device()
							->CreateArrayBuffer<Graphics::AccelerationStructureInstanceDesc>(count, Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY);
						if (m_kernels.instanceDescriptors->BoundObject() == nullptr ||
							m_kernels.instanceDescriptors->BoundObject()->Size() < minRequiredBufferSize) {
							return fail("Failed to obtain a buffer for the instance descriptors! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						}
					}
				}

				// Obtain the command buffer:
				const Graphics::InFlightBufferInfo commandBuffer = reader.Context()->Graphics()->GetWorkerThreadCommandBuffer();
				if (commandBuffer.commandBuffer == nullptr)
					return fail("Could not obtain a valid command buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				// Obtain blas reference buffers:
				if (m_kernels.blasReferenceStagingBuffer.size() <= commandBuffer.inFlightBufferId)
					m_kernels.blasReferenceStagingBuffer.resize(commandBuffer.inFlightBufferId + 1u);
				Graphics::ArrayBufferReference<uint64_t>& blasReferenceStagingBuffer = m_kernels.blasReferenceStagingBuffer[commandBuffer.inFlightBufferId];
				{
					const size_t minRequiredEntryCount = Math::Max(indirectBlasReferenceCount, size_t(1u));
					const size_t minRequiredBufferSize = sizeof(uint64_t) * minRequiredEntryCount;

					if (blasReferenceStagingBuffer == nullptr ||
						blasReferenceStagingBuffer->Size() < minRequiredBufferSize) {
						size_t count = 1u;
						while (count < minRequiredEntryCount)
							count <<= 1u;
						blasReferenceStagingBuffer = reader.Context()->Graphics()->Device()
							->CreateArrayBuffer<uint64_t>(count, Graphics::Buffer::CPUAccess::CPU_READ_WRITE);
						m_kernels.blasReferenceBuffer->BoundObject() = blasReferenceStagingBuffer;
						if (blasReferenceStagingBuffer == nullptr ||
							blasReferenceStagingBuffer->Size() < minRequiredBufferSize) {
							return fail("Failed to allocate staging buffer for the indirect blas-references! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						}
					}

					if (m_kernels.blasReferenceBuffer->BoundObject() == nullptr ||
						m_kernels.blasReferenceBuffer->BoundObject()->Size() < minRequiredBufferSize) {
						m_kernels.blasReferenceBuffer->BoundObject() = m_kernels.transientBuffers->GetBuffer(minRequiredBufferSize, 1u);
						if (m_kernels.blasReferenceBuffer->BoundObject() == nullptr) {
							return fail("Failed to obtain transient buffer for the indirect blas-references! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						}
					}
				}

				// Update indirect blas references:
				if (indirectBlasReferenceCount > 0u) {
					uint64_t* const blasRefs = blasReferenceStagingBuffer.Map();
					for (size_t i = 0u; i < instanceGeneratorSettings.size(); i++) {
						const InstanceGeneratorSettings& instances = instanceGeneratorSettings[i];
						const size_t firstBlasIndex = objectInformation[i].firstBlas;
						if (instances.blasCount <= 1u)
							continue;
						for (size_t j = 0u; j < instances.blasCount; j++)
							blasRefs[instances.blasReference + j] = blasses[firstBlasIndex + j]->DeviceAddress();
					}
					blasReferenceStagingBuffer->Unmap(true);
					if (m_kernels.blasReferenceBuffer->BoundObject() != blasReferenceStagingBuffer)
						m_kernels.blasReferenceBuffer->BoundObject()->Copy(
							commandBuffer, blasReferenceStagingBuffer, sizeof(uint64_t) * indirectBlasReferenceCount);
				}

				// Calculate live ranges:
				if (liveRangeCalculationSettings.size() > 0u) {
					m_kernels.liveRangeBuffer->BoundObject()->Fill(commandBuffer, 0u, sizeof(uint32_t) * segmentTreeBufferSize, 0u);
					m_kernels.liveRangeKernel->Execute(
						commandBuffer, liveRangeCalculationSettings.data(), liveRangeCalculationSettings.size());
					const Reference<Graphics::ArrayBuffer> segmentTree = m_kernels.segementTreeKernel->Execute(
						commandBuffer, m_kernels.liveRangeBuffer->BoundObject(), segmentTreeSize, true);
					if (segmentTree != m_kernels.liveRangeBuffer->BoundObject())
						return fail("Filled segment-tree buffer expected to be the same as the input buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				}

				// Build instance buffers:
				if (instanceGeneratorSettings.size() > 0u)
					m_kernels.instanceGeneratorKernel->Execute(
						commandBuffer, instanceGeneratorSettings.data(), instanceGeneratorSettings.size());

				// Allocate TLAS if needed:
				if (m_tlas == nullptr || m_blasInstanceBudget < totalInstanceCount) {
					while (m_blasInstanceBudget < totalInstanceCount)
						m_blasInstanceBudget <<= 1u;
					Graphics::TopLevelAccelerationStructure::Properties tlasProps = {};
					{
						tlasProps.maxBottomLevelInstances = static_cast<uint32_t>(m_blasInstanceBudget);
						tlasProps.flags = Graphics::AccelerationStructure::Flags::PREFER_FAST_BUILD;
					}
					m_tlas = reader.Context()->Graphics()->Device()->CreateTopLevelAccelerationStructure(tlasProps);
					if (m_tlas == nullptr)
						return fail("Failed to allocate TLAS! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				}

				// Build TLAS:
				m_tlas->Build(commandBuffer, m_kernels.instanceDescriptors->BoundObject(), nullptr, totalInstanceCount, 0u);

				// Update active snapshot:
				{
					std::unique_lock<decltype(m_tlasSnapshots->activeSnapshotLock)> activeSnapshotSelectionLock(m_tlasSnapshots->activeSnapshotLock);
					assert(m_tlasSnapshots->inFlightSnapshots.size() > commandBuffer.inFlightBufferId);
					m_tlasSnapshots->activeSnapshot = m_tlasSnapshots->inFlightSnapshots[commandBuffer];
					assert(m_tlasSnapshots->activeSnapshot != nullptr);
					std::unique_lock<decltype(m_tlasSnapshots->activeSnapshot->lock)> activeSnapshotLock(m_tlasSnapshots->activeSnapshot->lock);
					m_tlasSnapshots->activeSnapshot->objectInformation = std::move(objectInformation);
					m_tlasSnapshots->activeSnapshot->blasses = std::move(blasses);
					m_tlasSnapshots->activeSnapshot->tlas = m_tlas;
				}
			}

			virtual void CollectDependencies(Callback<Job*> addDependency) {
				// We can run once blas-es are collected and built for this frame:
				m_blasProvider->CollectBuildJobs(addDependency);
				addDependency(m_blasCollector);
			}

		public:
			inline static Reference<TlasBuilder> Create(const Descriptor& desc, 
				SceneAccelerationStructures* blasProvider, 
				BlasCollector* blasCollector,
				TlasSnapshots* tlasSnapshots) {

				auto fail = [&](const auto&... message) {
					desc.descriptorSet->Context()->Log()->Error(
						"GraphicsObjectAccelerationStructure::Helpers::TlasBuilder::Create - ", message...);
					return nullptr;
				};

				Kernels kernels;

				// Transient buffer set:
				kernels.transientBuffers = Graphics::TransientBufferSet::Get(desc.descriptorSet->Context()->Graphics()->Device());
				if (kernels.transientBuffers == nullptr)
					return fail("Failed to obtain transient buffers! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				// Bindings:
				kernels.liveRangeBuffer = Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
				kernels.instanceDescriptors = Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
				kernels.blasReferenceBuffer = Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
				Graphics::BindingSet::BindingSearchFunctions bindings = {};
				auto findStructuredBuffers = [&](const Graphics::BindingSet::BindingDescriptor& desc) 
					-> Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> {
					if (desc.name == "liveRangeMarkers" ||
						desc.name == "segmentTreeBuffer")
						return kernels.liveRangeBuffer;
					else if (desc.name == "instanceDescriptors")
						return kernels.instanceDescriptors;
					else if (desc.name == "blasReferenceBuffer")
						return kernels.blasReferenceBuffer;
					else return nullptr;
				};
				bindings.structuredBuffer = &findStructuredBuffers;

				// Create live-range kernel:
				static const constexpr std::string_view LIVE_RANGE_KERNEL_PATH(
					"Jimara/Environment/Rendering/LightingModels/Utilities/GraphicsObjectAccelerationStructure_LiveRanges.comp");
				kernels.liveRangeKernel = LiveRangeKernel::Create(
					desc.descriptorSet->Context(), LIVE_RANGE_KERNEL_PATH, bindings);
				if (kernels.liveRangeKernel == nullptr)
					return fail("Failed to create live-range kernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				// Create segment-tree calculator kernel:
				kernels.segementTreeKernel = SegmentTreeGenerationKernel::CreateIntSumKernel(
					desc.descriptorSet->Context()->Graphics()->Device(),
					desc.descriptorSet->Context()->Graphics()->Configuration().ShaderLibrary(),
					desc.descriptorSet->Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount());
				if (kernels.segementTreeKernel == nullptr)
					return fail("Failed to create segment-tree kernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				// Create instance generator kernel:
				static const constexpr std::string_view INSTANCE_GENERATOR_KERNEL_PATH(
					"Jimara/Environment/Rendering/LightingModels/Utilities/GraphicsObjectAccelerationStructure_InstanceGenerator.comp");
				kernels.instanceGeneratorKernel = InstanceGeneratorKernel::Create(
					desc.descriptorSet->Context(), INSTANCE_GENERATOR_KERNEL_PATH, bindings);
				if (kernels.instanceGeneratorKernel == nullptr)
					return fail("Failed to create instance generator kernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				// Done:
				return Object::Instantiate<TlasBuilder>(blasProvider, blasCollector, &kernels, tlasSnapshots);
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

		static void OnCollectBlasBuildDependencies(BlasCollector* collector, Callback<JobSystem::Job*> collect) {
			collect(collector);
		}
		static Callback<Callback<JobSystem::Job*>> OnCollectBlasBuildDependenciesCallback(BlasCollector* collector) {
			return Callback<Callback<JobSystem::Job*>>(GraphicsObjectAccelerationStructure::Helpers::OnCollectBlasBuildDependencies, collector);
		}

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
					return fail("Can not obtain simulation jobs! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				// Get BLAS builder:
				m_blasBuilder = SceneAccelerationStructures::Get(desc.descriptorSet->Context());
				if (m_blasBuilder == nullptr)
					return fail("Can not obtain SceneAccelerationStructures! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				// Create jobs:
				m_blasCollector = Object::Instantiate<BlasCollector>(simulationJobs, m_blasBuilder, m_objectSet);
				m_blasBuilder->OnCollectBuildDependencies() += OnCollectBlasBuildDependenciesCallback(m_blasCollector);
				m_tlasBuilder = TlasBuilder::Create(desc, m_blasBuilder, m_blasCollector, m_tlasSnapshots);
				if (m_tlasBuilder == nullptr)
					return fail("Failed to create TLAS builder! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				
				
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
				if (m_blasBuilder != nullptr && m_blasCollector != nullptr)
					m_blasBuilder->OnCollectBuildDependencies() -= OnCollectBlasBuildDependenciesCallback(m_blasCollector);
				m_blasCollector = nullptr;
				m_blasBuilder = nullptr;

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
			Reference<SceneAccelerationStructures> m_blasBuilder;
			Reference<BlasCollector> m_blasCollector;
			Reference<TlasBuilder> m_tlasBuilder;
			Reference<ScheduledJob> m_scheduledJob;
			const Reference<TlasSnapshots> m_tlasSnapshots = Object::Instantiate<TlasSnapshots>();

			friend class GraphicsObjectAccelerationStructure;
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

	void GraphicsObjectAccelerationStructure::CollectBuildJobs(const Callback<JobSystem::Job*> report)const {
		const Helpers::Instance* self = dynamic_cast<const Helpers::Instance*>(this);
		assert(self != nullptr);
		// No locking necessary, we don't initialize outside the GetFor() scope:
		report(self->m_tlasBuilder);
	}

	GraphicsObjectAccelerationStructure::Reader::Reader(const GraphicsObjectAccelerationStructure* accelerationStructure) {
		const Helpers::Instance* self = dynamic_cast<const Helpers::Instance*>(accelerationStructure);
		if (self == nullptr)
			return;

		// Obtain snapshot:
		Reference<Helpers::TlasSnapshot> snapshot;
		{
			assert(self->m_tlasSnapshots != nullptr);
			std::unique_lock<decltype(self->m_tlasSnapshots->activeSnapshotLock)> lock(self->m_tlasSnapshots->activeSnapshotLock);
			snapshot = self->m_tlasSnapshots->activeSnapshot;
		}
		if (snapshot == nullptr)
			return;

		// Lock snapshot modification:
		snapshot->lock.lock_shared();
		m_as = snapshot;
		assert(m_as != nullptr);

		// Fetch snapshot data:
		m_tlas = snapshot->tlas;
		m_objectCount = snapshot->objectInformation.size();
		m_objects = snapshot->objectInformation.data();
		m_blasCount = snapshot->blasses.size();
		m_blasses = snapshot->blasses.data();
		m_rayTransform = snapshot->rayTransform;
	}
	GraphicsObjectAccelerationStructure::Reader::~Reader() {
		// Unlock snapshot modification:
		Reference<Helpers::TlasSnapshot> snapshot = m_as;
		if (snapshot != nullptr)
			snapshot->lock.unlock_shared();
		m_as = nullptr;
	}
}

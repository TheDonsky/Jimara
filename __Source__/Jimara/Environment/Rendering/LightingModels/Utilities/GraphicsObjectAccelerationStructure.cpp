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
				std::shared_lock<decltype(m_dataLock)> m_lock;
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
			std::vector<GraphicsObjectGeometry> m_objectGeometry;
			std::vector<Reference<SceneAccelerationStructures::Blas>> m_oldBlasInstances;
			std::vector<Reference<SceneAccelerationStructures::Blas>> m_blasInstances;
			
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
			}
			inline virtual ~BlasCollector() {}

		protected:
			// __TODO__: This class is supposed to read GraphicsObjectSet each frame and fill a corresponding list of Blas instances.
			// Should preferrably run before scene acceleration structures get built.
			// It should also wait for the graphics simulation to finish.

			virtual void Execute() {
				GraphicsObjectSet::Reader objectSet(m_objectSet);

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
		};

		class TlasBuilder : public virtual JobSystem::Job {
		private:
			const Reference<SceneAccelerationStructures> m_blasProvider;
			const Reference<BlasCollector> m_blasCollector;
			// __TODO__: This class is supposed to run after the BLAS instances are updated and should build the TLAS from them.

		public:
			inline TlasBuilder(SceneAccelerationStructures* blasProvider, BlasCollector* blasCollector)
				: m_blasProvider(blasProvider), m_blasCollector(blasCollector) {
				assert(m_blasProvider != nullptr);
				assert(m_blasCollector != nullptr);
			}
			inline virtual ~TlasBuilder() {}

		protected:
			virtual void Execute() {
				// __TODO__: Implement this crap! [EI BUILD TLASS instance buffer and the TLAS itself]!!
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

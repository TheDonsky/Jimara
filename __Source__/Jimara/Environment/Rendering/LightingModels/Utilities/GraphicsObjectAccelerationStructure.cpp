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
			mutable JM_StandardVertexInput::Extractor vertexInputBindings;

			inline GraphicsObjectData(GraphicsObjectDescriptor* desc = nullptr) : graphicsObject(desc) {}

			inline ~GraphicsObjectData() {}
		};

		class GraphicsObjectSet : public virtual Object {
		private:
			std::mutex m_descriptorLock;
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

					// Obtain vertex inputs:
					JM_StandardVertexInput::Extractor vertexInput(data);

					// We need index buffer:
					if (vertexInput.IndexBuffer() == nullptr)
						continue;

					// We need vertex position input:
					if (vertexInput.VertexPosition().bufferBinding == nullptr)
						continue;
					else if ((vertexInput.VertexPosition().flags & (
						JM_StandardVertexInput::Flags::FIELD_RATE_PER_VERTEX |
						JM_StandardVertexInput::Flags::FIELD_RATE_PER_INSTANCE))
						!= JM_StandardVertexInput::Flags::FIELD_RATE_PER_VERTEX) {
						// __TODO__: Maybe we need a different kind of handling here?
						continue;
					}

					// We need per-instance transform input:
					if (vertexInput.ObjectTransform().bufferBinding == nullptr)
						continue;
					else if ((vertexInput.ObjectTransform().flags & (
						JM_StandardVertexInput::Flags::FIELD_RATE_PER_VERTEX |
						JM_StandardVertexInput::Flags::FIELD_RATE_PER_INSTANCE))
						!= JM_StandardVertexInput::Flags::FIELD_RATE_PER_INSTANCE) {
						// __TODO__: Maybe we need a different kind of handling here?
						continue;
					}

					// Add to graphics object data:
					m_graphicsObjectData.Add(&desc, 1u, [&](const GraphicsObjectData* inserted, size_t insertedCount) {
						if (insertedCount <= 0u)
							return;
						assert(insertedCount == 1u);
						inserted->viewportData = data;
						inserted->vertexInputBindings = vertexInput;
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

				m_desc = desc;
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
				}
				m_desc.descriptorSet = nullptr;
				m_desc = {};
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
			};
		};

		struct GraphicsObjectBlas {
			Reference<const GraphicsObjectDescriptor> graphicsObject;
			Reference<const GraphicsObjectDescriptor::ViewportData> viewportData;

			Reference<const SceneAccelerationStructures::Blas> oldBlas;
			Reference<const SceneAccelerationStructures::Blas> blas;

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
			std::vector<GraphicsObjectBlas> m_blasInstances;
			
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
				if (m_blasInstances.size() < objectSet.Size())
					m_blasInstances.resize(objectSet.Size());

				for (size_t i = 0u; i < objectSet.Size(); i++) {
					const GraphicsObjectData& data = objectSet[i];
					GraphicsObjectBlas& blasData = m_blasInstances[i];

					// Copy graphics object info:
					blasData.graphicsObject = data.graphicsObject;
					blasData.viewportData = data.viewportData;

					// Keep old blas:
					blasData.oldBlas = blasData.blas;

					// Define blas descriptor:
					SceneAccelerationStructures::BlasDesc blasDesc = {};
					{
						assert(data.vertexInputBindings.VertexPosition().bufferBinding != nullptr);
						blasDesc.vertexBuffer = data.vertexInputBindings.VertexPosition().bufferBinding->BoundObject();
						assert(data.vertexInputBindings.IndexBuffer() != nullptr);
						blasDesc.indexBuffer = data.vertexInputBindings.IndexBuffer()->BoundObject();
						blasDesc.vertexFormat = Graphics::BottomLevelAccelerationStructure::VertexFormat::X32Y32Z32;
						blasDesc.indexFormat =
							(blasDesc.indexBuffer == nullptr || blasDesc.indexBuffer->ObjectSize() == sizeof(uint16_t))
							? Graphics::BottomLevelAccelerationStructure::IndexFormat::U16
							: Graphics::BottomLevelAccelerationStructure::IndexFormat::U32;
						blasDesc.vertexPositionOffset = data.vertexInputBindings.VertexPosition().elemOffset;
						blasDesc.vertexStride = data.vertexInputBindings.VertexPosition().elemStride;
						blasDesc.vertexCount = // __TODO__: This will need to be updated...
							(blasDesc.vertexBuffer == nullptr) ? size_t(0u) :
							(Math::Max(blasDesc.vertexBuffer->Size(), size_t(blasDesc.vertexPositionOffset)) - blasDesc.vertexPositionOffset) /
							Math::Max(blasDesc.vertexStride, uint32_t(1u));
						assert(data.viewportData != nullptr);
						blasDesc.faceCount = data.viewportData->IndexCount() / 3u;
						blasDesc.faceOffset = 0u;
						blasDesc.flags = SceneAccelerationStructures::Flags::NONE; // __TODO__: We need per-frame updates for dynamic objects!
						blasDesc.displacementJob = Unused<Graphics::CommandBuffer*, uint64_t>;
						blasDesc.displacementJobId = 0u;
					}

					// If blas descriptor is 'empty'/invalid, we can ignore the entry:
					if (blasDesc.vertexBuffer == nullptr ||
						blasDesc.indexBuffer == nullptr ||
						blasDesc.vertexCount <= 0u ||
						blasDesc.faceCount <= 0u)
						blasData.blas = nullptr;

					// If we have a new blas-descriptor, we need to update the BLAS:
					else if (blasData.blas == nullptr || blasData.blas->Descriptor() != blasDesc)
						blasData.blas = m_blasProvider->GetBlas(blasDesc);

					// Instance data:
					{
						assert(data.viewportData != nullptr);
						blasData.indirectDrawCommands = data.viewportData->IndirectBuffer();
						assert(data.vertexInputBindings.ObjectTransform().bufferBinding != nullptr);
						blasData.instanceTransforms = data.vertexInputBindings.ObjectTransform().bufferBinding->BoundObject();
						blasData.instanceTransformOffset = data.vertexInputBindings.ObjectTransform().elemOffset;
						blasData.instanceTransformSride = data.vertexInputBindings.ObjectTransform().elemStride;
						blasData.drawCount = data.viewportData->InstanceCount();
					}
				}

				// Resize if we have extra entries:
				if (m_blasInstances.size() > objectSet.Size())
					m_blasInstances.resize(objectSet.Size());
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
		// __TODO__: Implement this crap!.
		return nullptr;
	}
}

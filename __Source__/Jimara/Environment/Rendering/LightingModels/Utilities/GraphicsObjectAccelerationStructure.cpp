#include "GraphicsObjectAccelerationStructure.h"


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

		struct BlasCollector : public virtual JobSystem::Job {
			// __TODO__: This class is supposed to read GraphicsObjectSet each frame and fill a corresponding list of Blas instances.
			// Should preferrably run before scene acceleration structures get built.
			// It should also wait for the graphics simulation to finish.

			virtual void Execute() {
				// __TODO__: Implement this crap!
			}

			virtual void CollectDependencies(Callback<Job*> addDependency) {
				// __TODO__: Implement this crap!
			}
		};

		struct TlasBuilder : public virtual JobSystem::Job {
			// __TODO__: This class is supposed to run after the BLAS instances are updated and should build the TLAS from them.

			virtual void Execute() {
				// __TODO__: Implement this crap!
			}

			virtual void CollectDependencies(Callback<Job*> addDependency) {
				// __TODO__: Implement this crap!
			}
		};

		class Instance : public virtual GraphicsObjectAccelerationStructure {
		public:
			inline Instance() {
				assert(m_objectSet != nullptr);
			}

			inline virtual ~Instance() {
				Clear();
			}

			inline bool Initialize(const Descriptor& desc) {
				m_objectSet->Initialize(desc);
			}

			inline void Clear() {
				m_objectSet->Clear();
			}

		private:
			const Reference<GraphicsObjectSet> m_objectSet = Object::Instantiate<GraphicsObjectSet>();
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

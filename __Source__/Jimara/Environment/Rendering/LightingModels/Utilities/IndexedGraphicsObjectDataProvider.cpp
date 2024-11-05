#include "IndexedGraphicsObjectDataProvider.h"


namespace Jimara {
	struct IndexedGraphicsObjectDataProvider::Helpers {
		// Each material binding set will need an additional binding with custom[indirect] index; this is the index:
		struct ObjetIndex final {
			Reference<const Graphics::ResourceBinding<Graphics::Buffer>> binding;
			uint32_t index = 0u;
		};

		// ObjetIndirectionId entries will be retrieved, freed-into and reallocated from this object:
		class ObjetIndexPool : public virtual Object {
		public:
			const Descriptor descriptor;

		private:
			SpinLock m_lock;
			std::vector<ObjetIndex> m_freeBuffers;
			std::atomic<uint32_t> m_allocatedBufferCounter = 0u;

		public:
			inline ObjetIndexPool(const Descriptor& desc)
				: descriptor(desc) {}
			inline virtual ~ObjetIndexPool() {
				if (m_allocatedBufferCounter != m_freeBuffers.size())
					descriptor.graphicsObjects->Context()->Log()->Error(
						"IndexedGraphicsObjectDataProvider::Helpers::ObjetIndexPool::~ObjetIndexPool - ",
						"not all bindings freed! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			inline ObjetIndex GetBinding() {
				// Get cached
				{
					std::unique_lock<decltype(m_lock)> lock(m_lock);
					if (!m_freeBuffers.empty()) {
						ObjetIndex res = m_freeBuffers.back();
						m_freeBuffers.pop_back();
						return res;
					}
				}

				// Create new:
				ObjetIndex id = {};
				id.index = m_allocatedBufferCounter.fetch_add(1u);
				if (id.index >= ~uint32_t(0u)) {
					descriptor.graphicsObjects->Context()->Log()->Error(
						"IndexedGraphicsObjectDataProvider::Helpers::ObjetIndexPool::GetBinding - ",
						"allocatedBufferCounter overflow detected! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return {};
				}
				const Graphics::BufferReference<uint32_t> buffer = 
					descriptor.graphicsObjects->Context()->Graphics()->Device()->CreateConstantBuffer<uint32_t>();
				if (buffer == nullptr) {
					descriptor.graphicsObjects->Context()->Log()->Error(
						"IndexedGraphicsObjectDataProvider::Helpers::ObjetIndexPool::GetBinding - ",
						"Failed to allocate indirection buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return {};
				}
				buffer.Map() = id.index;
				buffer->Unmap(true);
				id.binding = Object::Instantiate<const Graphics::ResourceBinding<Graphics::Buffer>>(buffer);
				return id;
			}

			inline void ReleaseBinding(const ObjetIndex& id) {
				std::unique_lock<decltype(m_lock)> lock(m_lock);
				assert(id.binding != nullptr);
				m_freeBuffers.push_back(id);
			}
		};

#pragma warning(disable: 4250)
		// Custom vieport data wraps around the regular viewport data and also exposes a dedicated ObjetIndirectionId instance:
		class CustomViewportData
			: public virtual ViewportData
			, public virtual ObjectCache<Reference<const Object>>::StoredObject {
		private:
			const Reference<ObjetIndexPool> m_indexPool;
			const Reference<const Graphics::ResourceBinding<Graphics::Buffer>> m_binding;
			const Graphics::BindingSet::BindingSearchFunctions m_baseBindingSearchFunctions;

		public:
			inline CustomViewportData(
				const GraphicsObjectDescriptor::ViewportData* underlyingData, 
				ObjetIndexPool* indexPool, ObjetIndex& index)
				: GraphicsObjectDescriptor::ViewportData(underlyingData->GeometryType())
				, IndexedGraphicsObjectDataProvider::ViewportData(underlyingData, index.index)
				, m_indexPool(indexPool)
				, m_binding(index.binding)
				, m_baseBindingSearchFunctions(underlyingData->BindingSearchFunctions()) {}

			inline virtual ~CustomViewportData() {
				ObjetIndex index = {};
				index.binding = m_binding;
				index.index = Index();
				m_indexPool->ReleaseBinding(index);
			}

			Reference<const Graphics::ResourceBinding<Graphics::Buffer>> FindConstantBuffer(const Graphics::BindingSet::BindingDescriptor& desc)const {
				if (desc.name == m_indexPool->descriptor.customIndexBindingName)
					return m_binding;
				else return m_baseBindingSearchFunctions.constantBuffer(desc);
			}

			inline virtual Graphics::BindingSet::BindingSearchFunctions BindingSearchFunctions()const final override {
				Graphics::BindingSet::BindingSearchFunctions functions = m_baseBindingSearchFunctions;
				functions.constantBuffer = Function<
					Reference<const Graphics::ResourceBinding<Graphics::Buffer>>,
					const Graphics::BindingSet::BindingDescriptor&>
					(&CustomViewportData::FindConstantBuffer, this);
				return functions;
			}

			inline virtual GraphicsObjectDescriptor::VertexInputInfo VertexInput()const final override { return BaseData()->VertexInput(); }
			inline virtual Graphics::IndirectDrawBufferReference IndirectBuffer()const final override { return BaseData()->IndirectBuffer(); }
			inline virtual size_t IndexCount()const final override { return BaseData()->IndexCount(); }
			inline virtual size_t InstanceCount()const final override { return BaseData()->InstanceCount(); }
			inline virtual Reference<Component> GetComponent(size_t objectIndex)const final override { return BaseData()->GetComponent(objectIndex); }
		};


		// Cached IndexedGraphicsObjectDataProvider:
		class CachedDataProvider
			: public virtual IndexedGraphicsObjectDataProvider
			, public virtual ObjectCache<Reference<const Object>>
			, public virtual ObjectCache<Descriptor>::StoredObject {
		private:
			const Reference<ObjetIndexPool> m_indexPool;

		public:
			inline CachedDataProvider(const Descriptor& desc)
				: m_indexPool(Object::Instantiate<ObjetIndexPool>(desc)) {}

			virtual Reference<const GraphicsObjectDescriptor::ViewportData> GetViewportData(
				GraphicsObjectDescriptor* graphicsObject, const RendererFrustrumDescriptor* frustrum) final override {
				if (graphicsObject == nullptr)
					return nullptr;
				const Reference<const GraphicsObjectDescriptor::ViewportData> baseData = graphicsObject->GetViewportData(frustrum);
				if (baseData == nullptr)
					return nullptr;
				const Reference<ViewportData> instance = GetCachedOrCreate(baseData, [&]() -> Reference<CustomViewportData> {
					ObjetIndex index = m_indexPool->GetBinding();
					if (index.binding == nullptr)
						return nullptr;
					return Object::Instantiate<CustomViewportData>(baseData, m_indexPool, index);
					});
				return instance;
			}

			inline virtual ~CachedDataProvider() {}
		};

		// Cache for IndexedGraphicsObjectDataProvider instances:
		class ProviderCache : public virtual ObjectCache<Descriptor> {
		public:
			inline Reference<IndexedGraphicsObjectDataProvider> GetFor(const Descriptor& desc) {
				if (desc.graphicsObjects == nullptr)
					return nullptr;
				else return GetCachedOrCreate(desc, [&]() { return Object::Instantiate<CachedDataProvider>(desc); });
			};
		};

#pragma warning(default: 4250)
	};


	Reference<IndexedGraphicsObjectDataProvider> IndexedGraphicsObjectDataProvider::GetFor(const Descriptor& id) {
		static Helpers::ProviderCache cache;
		return cache.GetFor(id);
	}

	bool IndexedGraphicsObjectDataProvider::Descriptor::operator==(const Descriptor& other)const {
		return
			graphicsObjects == other.graphicsObjects &&
			frustrumDescriptor == other.frustrumDescriptor &&
			customIndexBindingName == other.customIndexBindingName;
	}

	bool IndexedGraphicsObjectDataProvider::Descriptor::operator!=(const Descriptor& other)const {
		return !((*this) == other);
	}
}

namespace std {
	size_t hash<Jimara::IndexedGraphicsObjectDataProvider::Descriptor>::operator()(const Jimara::IndexedGraphicsObjectDataProvider::Descriptor& desc)const {
		return Jimara::MergeHashes(
			std::hash<const Jimara::Object*>()(desc.graphicsObjects),
			std::hash<const Jimara::Object*>()(desc.frustrumDescriptor),
			std::hash<std::string>()(desc.customIndexBindingName));
	}
}

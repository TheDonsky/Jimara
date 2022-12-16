#include "ViewportLightSet.h"


namespace Jimara {
	struct ViewportLightSet::Helpers {
		class CachedSet;

		struct ViewportDataReference {
			Reference<LightDescriptor> objectDescriptor;
			mutable Reference<const LightDescriptor::ViewportData> viewportData;

			inline ViewportDataReference(LightDescriptor* descriptor = nullptr) : objectDescriptor(descriptor) {}
		};

		class PerViewportData : public virtual Object {
		private:
			const Reference<SceneContext> m_context;
			const Reference<const ViewportDescriptor> m_viewport;
			const Reference<LightDescriptor::Set> m_descriptors;

			const std::shared_ptr<SpinLock> m_ownerLock;
			mutable CachedSet* m_owner;
			friend class CachedSet;

		public:
			std::shared_mutex descriptorSetLock;
			ObjectSet<LightDescriptor, ViewportDataReference> descriptorSet;

		private:
			template<typename ReferenceType>
			inline void AddDescriptors(const ReferenceType* descriptors, size_t count) {
				descriptorSet.Add(descriptors, count, [&](const ViewportDataReference* const entries, size_t numEntries) {
					for (size_t i = 0; i < numEntries; i++)
						entries[i].viewportData = entries[i].objectDescriptor->GetViewportData(m_viewport);
					});
			}

			inline void OnDescriptorsAdded(LightDescriptor* const* descriptors, size_t count) {
				std::unique_lock<std::shared_mutex> lock(descriptorSetLock);
				AddDescriptors(descriptors, count);
			}

			inline void OnDescriptorsRemoved(LightDescriptor* const* descriptors, size_t count) {
				std::unique_lock<std::shared_mutex> lock(descriptorSetLock);
				descriptorSet.Remove(descriptors, count);
			}

		protected:
			inline virtual void OnOutOfScope()const override {
				std::shared_ptr<SpinLock> ownerLock = m_ownerLock;
				std::unique_lock<SpinLock> lock(*ownerLock);
				if (RefCount() > 0) return;
				if (m_owner != nullptr) {
					m_owner->m_data = nullptr;
					m_owner = nullptr;
				}
				Object::OnOutOfScope();
			}

		public:
			inline PerViewportData(const ViewportDescriptor* viewport, const std::shared_ptr<SpinLock>* ownerLock, CachedSet* owner)
				: m_context(owner->m_context)
				, m_viewport(viewport)
				, m_descriptors(LightDescriptor::Set::GetInstance(owner->m_context))
				, m_ownerLock(*ownerLock)
				, m_owner(owner) {
				m_descriptors->OnAdded() += Callback(&PerViewportData::OnDescriptorsAdded, this);
				m_descriptors->OnRemoved() += Callback(&PerViewportData::OnDescriptorsRemoved, this);
				{
					std::unique_lock<std::shared_mutex> lock(descriptorSetLock);
					std::vector<Reference<LightDescriptor>> descriptors;
					m_descriptors->GetAll([&](LightDescriptor* descriptor) { descriptors.push_back(descriptor); });
					AddDescriptors(descriptors.data(), descriptors.size());
				}
			}

			inline virtual ~PerViewportData() {
				m_descriptors->OnAdded() -= Callback(&PerViewportData::OnDescriptorsAdded, this);
				m_descriptors->OnRemoved() -= Callback(&PerViewportData::OnDescriptorsRemoved, this);
				assert(m_owner == nullptr);
			}
		};

#pragma warning(disable: 4250)
		class CachedSet : public virtual ViewportLightSet, public virtual ObjectCache<Reference<const Object>>::StoredObject {
		private:
			const Reference<SceneContext> m_context;
			const std::shared_ptr<SpinLock> m_dataLock = std::make_shared<SpinLock>();
			PerViewportData* m_data = nullptr;

			friend class PerViewportData;

		public:
			inline CachedSet(SceneContext* context, const ViewportDescriptor* viewport) 
				: m_context(context) {
				const Reference<PerViewportData> viewportData = Object::Instantiate<PerViewportData>(viewport, &m_dataLock, this);
				m_data = viewportData;
				m_context->StoreDataObject(viewportData);
			}

			inline virtual ~CachedSet() {
				Reference<PerViewportData> viewportData;
				{
					std::unique_lock<SpinLock> lock(*m_dataLock);
					viewportData = m_data;
					if (viewportData != nullptr)
						viewportData->m_owner = nullptr;
					m_data = nullptr;
				};
				if (viewportData != nullptr)
					m_context->EraseDataObject(viewportData);
			}

			inline Reference<PerViewportData> GetData()const {
				std::unique_lock<SpinLock> lock(*m_dataLock);
				const Reference<PerViewportData> rv = m_data;
				return rv;
			}
		};

		class Cache : public virtual ObjectCache<Reference<const Object>> {
		public:
			inline static Reference<const ViewportLightSet> GetFor(const Object* key, SceneContext* context, const ViewportDescriptor* viewport) {
				static Cache cache;
				return cache.GetCachedOrCreate(key, false, [&]() -> Reference<CachedSet> { return Object::Instantiate<CachedSet>(context, viewport); });
			}
		};
#pragma warning(default: 4250)
	};

	Reference<const ViewportLightSet> ViewportLightSet::For(const ViewportDescriptor* viewport) {
		if (viewport == nullptr) return nullptr;
		else return Helpers::Cache::GetFor(viewport, viewport->Context(), viewport);
	}

	Reference<const ViewportLightSet> ViewportLightSet::For(SceneContext* context) {
		if (context == nullptr) return nullptr;
		else return Helpers::Cache::GetFor(context, context, nullptr);
	}


	ViewportLightSet::Reader::Reader(const ViewportLightSet* lightSet)
		: Reader(Reference<Object>((dynamic_cast<const Helpers::CachedSet*>(lightSet) != nullptr)
			? dynamic_cast<const Helpers::CachedSet*>(lightSet)->GetData() : nullptr)) {}

	ViewportLightSet::Reader::~Reader() {
		m_lock.unlock();
	}

	size_t ViewportLightSet::Reader::LightCount()const { return m_lightCount; }

	LightDescriptor* ViewportLightSet::Reader::LightDescriptor(size_t index)const {
		return reinterpret_cast<const Helpers::ViewportDataReference*>(m_descriptors)[index].objectDescriptor;
	}

	const LightDescriptor::ViewportData* ViewportLightSet::Reader::LightData(size_t index)const {
		return reinterpret_cast<const Helpers::ViewportDataReference*>(m_descriptors)[index].viewportData;
	}

	ViewportLightSet::Reader::Reader(const Reference<Object> dataObject) 
		: m_lock([&]()->std::shared_mutex& { 
		Helpers::PerViewportData* data = dynamic_cast<Helpers::PerViewportData*>(dataObject.operator->());
		if (data != nullptr) return data->descriptorSetLock;
		static std::shared_mutex defaultLock;
		return defaultLock;
			}())
		, m_data(dataObject) {
		const Helpers::PerViewportData* data = dynamic_cast<const Helpers::PerViewportData*>(m_data.operator->());
		if (data != nullptr) {
			const Helpers::ViewportDataReference* descriptors = data->descriptorSet.Data();
			m_descriptors = reinterpret_cast<const void*>(descriptors);
			m_lightCount = data->descriptorSet.Size();
		}
		else {
			m_descriptors = nullptr;
			m_lightCount = 0u;
		}
	}
}

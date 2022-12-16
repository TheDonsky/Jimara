#include "ViewportGraphicsObjectSet.h"


namespace Jimara {
	struct ViewportGraphicsObjectSet::Helpers {
		class CachedSet;

		struct ViewportDataReference {
			Reference<GraphicsObjectDescriptor> objectDescriptor;
			mutable Reference<const GraphicsObjectDescriptor::ViewportData> viewportData;

			inline ViewportDataReference(GraphicsObjectDescriptor* descriptor = nullptr) : objectDescriptor(descriptor) {}
		};

		inline static void InspectRegion(const ViewportDataReference* ptr, size_t count, const Callback<const ObjectInfo*, size_t>& inspectEntries) {
			static thread_local std::vector<ObjectInfo> info;
			info.clear();
			const Helpers::ViewportDataReference* const end = ptr + count;
			while (ptr < end) {
				info.push_back(ObjectInfo{ ptr->objectDescriptor.operator->(), ptr->viewportData.operator->() });
				ptr++;
			}
			inspectEntries(info.data(), info.size());
			info.clear();
		}

		template<typename InspectFn>
		inline static void InspectRegion(const ViewportDataReference* ptr, size_t count, const InspectFn& inspectFn) {
			InspectRegion(ptr, count, Callback<const ObjectInfo*, size_t>::FromCall(&inspectFn));
		};

		class PerViewportData : public virtual Object {
		private:
			const Reference<SceneContext> m_context;
			const Reference<const ViewportDescriptor> m_viewport;
			const Reference<GraphicsObjectDescriptor::Set> m_descriptors;

			const std::shared_ptr<EventInstance<const ObjectInfo*, size_t>> m_onAdded;
			const std::shared_ptr<EventInstance<const ObjectInfo*, size_t>> m_onRemoved;

			const std::shared_ptr<SpinLock> m_ownerLock;
			mutable CachedSet* m_owner;
			friend class CachedSet;

		public:
			std::shared_mutex descriptorSetLock;
			ObjectSet<GraphicsObjectDescriptor, ViewportDataReference> descriptorSet;

		private:
			template<typename ReferenceType>
			inline void AddDescriptors(const ReferenceType* descriptors, size_t count) {
				descriptorSet.Add(descriptors, count, [&](const ViewportDataReference* const entries, size_t numEntries) {
					for (size_t i = 0; i < numEntries; i++)
						entries[i].viewportData = entries[i].objectDescriptor->GetViewportData(m_viewport);
					InspectRegion(entries, numEntries, [&](const ObjectInfo* info, size_t count) { m_onAdded->operator()(info, count); });
					});
			}

			inline void OnDescriptorsAdded(GraphicsObjectDescriptor* const* descriptors, size_t count) {
				std::unique_lock<std::shared_mutex> lock(descriptorSetLock);
				AddDescriptors(descriptors, count);
			}

			inline void OnDescriptorsRemoved(GraphicsObjectDescriptor* const* descriptors, size_t count) {
				std::unique_lock<std::shared_mutex> lock(descriptorSetLock);
				descriptorSet.Remove(descriptors, count, [&](const ViewportDataReference* const entries, size_t numEntries) {
					InspectRegion(entries, numEntries, [&](const ObjectInfo* info, size_t count) { m_onRemoved->operator()(info, count); });
					});
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
				, m_descriptors(GraphicsObjectDescriptor::Set::GetInstance(owner->m_context))
				, m_onAdded(owner->m_onAdded)
				, m_onRemoved(owner->m_onRemoved)
				, m_ownerLock(*ownerLock)
				, m_owner(owner) {
				m_descriptors->OnAdded() += Callback(&PerViewportData::OnDescriptorsAdded, this);
				m_descriptors->OnRemoved() += Callback(&PerViewportData::OnDescriptorsRemoved, this);
				{
					std::unique_lock<std::shared_mutex> lock(descriptorSetLock);
					std::vector<Reference<GraphicsObjectDescriptor>> descriptors;
					m_descriptors->GetAll([&](GraphicsObjectDescriptor* descriptor) { descriptors.push_back(descriptor); });
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
		class CachedSet : public virtual ViewportGraphicsObjectSet, public virtual ObjectCache<Reference<const Object>>::StoredObject {
		private:
			const Reference<SceneContext> m_context;
			const std::shared_ptr<SpinLock> m_dataLock = std::make_shared<SpinLock>();
			PerViewportData* m_data = nullptr;

			friend class PerViewportData;

		public:
			const std::shared_ptr<EventInstance<const ObjectInfo*, size_t>> m_onAdded = std::make_shared<EventInstance<const ObjectInfo*, size_t>>();
			const std::shared_ptr<EventInstance<const ObjectInfo*, size_t>> m_onRemoved = std::make_shared<EventInstance<const ObjectInfo*, size_t>>();

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
			inline static Reference<const ViewportGraphicsObjectSet> GetFor(const Object* key, SceneContext* context, const ViewportDescriptor* viewport) {
				static Cache cache;
				return cache.GetCachedOrCreate(key, false, [&]() -> Reference<CachedSet> { return Object::Instantiate<CachedSet>(context, viewport); });
			}
		};
#pragma warning(default: 4250)
	};

	Reference<const ViewportGraphicsObjectSet> ViewportGraphicsObjectSet::For(const ViewportDescriptor* viewport) {
		if (viewport == nullptr) return nullptr;
		else return Helpers::Cache::GetFor(viewport, viewport->Context(), viewport);
	}

	Reference<const ViewportGraphicsObjectSet> ViewportGraphicsObjectSet::For(SceneContext* context) {
		if (context == nullptr) return nullptr;
		else return Helpers::Cache::GetFor(context, context, nullptr);
	}

	Event<const ViewportGraphicsObjectSet::ObjectInfo*, size_t>& ViewportGraphicsObjectSet::OnAdded()const {
		return *dynamic_cast<const Helpers::CachedSet*>(this)->m_onAdded;
	}

	Event<const ViewportGraphicsObjectSet::ObjectInfo*, size_t>& ViewportGraphicsObjectSet::OnRemoved()const {
		return *dynamic_cast<const Helpers::CachedSet*>(this)->m_onRemoved;
	}

	void ViewportGraphicsObjectSet::GetAll(const Callback<const ObjectInfo*, size_t>& inspectEntries)const {
		const Helpers::CachedSet* self = dynamic_cast<const Helpers::CachedSet*>(this);
		const Reference<Helpers::PerViewportData> data = self->GetData();
		if (data == nullptr) {
			inspectEntries(nullptr, 0u);
			return;
		}
		std::shared_lock<std::shared_mutex> lock(data->descriptorSetLock);
		Helpers::InspectRegion(data->descriptorSet.Data(), data->descriptorSet.Size(), inspectEntries);
	}
}

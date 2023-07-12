#include "AsynchronousActionQueue.h"
#include "../../Core/Collections/ThreadPool.h"

namespace Jimara {
	Reference<AsynchronousActionQueue> AsynchronousActionQueue::GetFor(SceneContext* context) {
		if (context == nullptr)
			return nullptr;

		class Data
			: public virtual Object
			, public virtual ThreadPool {
		public:
			class WeakPtr : public virtual Object {
			private:
				mutable SpinLock m_lock;
				mutable Data* m_data;

				friend class Data;

			public:
				inline WeakPtr(Data* data) : m_data(data) {}

				inline virtual ~WeakPtr() { 
					assert(m_data == nullptr);
				};

				inline Reference<Data> GetData() {
					std::unique_lock<SpinLock> lock(m_lock);
					Reference<Data> data = m_data;
					return data;
				}
			};
			const Reference<WeakPtr> weakPtr;

			inline Data() : weakPtr(Object::Instantiate<WeakPtr>(this)) {}

			inline virtual ~Data() {}

		protected:
			inline virtual void OnOutOfScope()const final override {
				Reference<WeakPtr> weakRef = weakPtr;
				std::unique_lock<SpinLock> lock(weakRef->m_lock);
				if (RefCount() > 0u) return;
				weakRef->m_data = nullptr;
				Object::OnOutOfScope();
			}
		};

#pragma warning(disable: 2450)
		class Instance 
			: public virtual AsynchronousActionQueue
			, public virtual ObjectCache<Reference<const Object>>::StoredObject {
		private:
			const Reference<SceneContext> m_context;
			const Reference<Data::WeakPtr> m_dataRef;

		public:
			inline Instance(SceneContext* context) 
				: m_context(context)
				, m_dataRef([&]() -> Reference<Data::WeakPtr> {
				const Reference<Data> data = Object::Instantiate<Data>();
				context->StoreDataObject(data);
				return data->weakPtr;
					}()) {}

			inline virtual ~Instance() {
				const Reference<Data> data = m_dataRef->GetData();
				if (data != nullptr)
					m_context->EraseDataObject(data);
			}

			inline virtual void Schedule(const Callback<Object*>& callback, Object* userData) final override {
				const Reference<Data> data = m_dataRef->GetData();
				if (data != nullptr)
					data->Schedule(callback, userData);
			}
		};
#pragma warning(default: 2450)

		class InstanceCache : public virtual ObjectCache<Reference<const Object>> {
		public:
			static Reference<AsynchronousActionQueue> Get(SceneContext* context) {
				static InstanceCache cache;
				static std::mutex creationLock;
				std::unique_lock<std::mutex> lock(creationLock);
				return cache.GetCachedOrCreate(context, false, [&]() {
					const Reference<Instance> instance = Object::Instantiate<Instance>(context);
					context->StoreDataObject(instance);
					return instance;
					});
			}
		};

		return InstanceCache::Get(context);
	}

	AsynchronousActionQueue::AsynchronousActionQueue() {}

	AsynchronousActionQueue::~AsynchronousActionQueue() {}
}

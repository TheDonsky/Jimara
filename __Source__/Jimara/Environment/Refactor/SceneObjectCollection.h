#pragma once
#include "Scene.h"
#include "../../Core/Collections/ObjectCache.h"
#include "../../Math/Helpers.h"


namespace Jimara {
#ifndef USE_REFACTORED_SCENE
namespace Refactor_TMP_Namespace {
#endif
	namespace SceneCachedInstances {
		/// <summary>
		/// Sometimes we need to store a single object of a type per scene context;
		/// this can be used to identify said object with relative ease
		/// </summary>
		struct InstanceId {
			/// <summary> SceneContext </summary>
			Reference<Scene::LogicContext> context;

			/// <summary> Type of the object we're interested in </summary>
			TypeId typeId;

			/// <summary> Default contructor </summary>
			inline InstanceId() {}

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="ctx"> SceneContext </param>
			/// <param name="type"> Type of the object we're interested in </param>
			inline InstanceId(Scene::LogicContext* ctx, const TypeId& type)
				: context(ctx), typeId(type) { }

			/// <summary> 'this equals other' </summary>
			inline bool operator==(const InstanceId& other)const {
				return context == other.context && typeId == other.typeId;
			}

			/// <summary> 'this is less than the other' </summary>
			inline bool operator<(const InstanceId& other)const {
				return context < other.context || (context == other.context && typeId < other.typeId);
			}
		};
	}
#ifndef USE_REFACTORED_SCENE
}
#endif
}

namespace std {
	/// <summary>
	/// Hash specialization for InstanceId
	/// </summary>
	template<>
	struct hash<
		Jimara::
#ifndef USE_REFACTORED_SCENE
		Refactor_TMP_Namespace::
#endif
		SceneCachedInstances::InstanceId> {
		/// <summary>
		/// Hash function
		/// </summary>
		/// <param name="id"> Id to hash </param>
		/// <returns> hash of the id </returns>
		inline size_t operator()(const Jimara::
#ifndef USE_REFACTORED_SCENE
			Refactor_TMP_Namespace::
#endif
			SceneCachedInstances::InstanceId& id)const {
			return Jimara::MergeHashes(std::hash<decltype(id.context)>()(id.context), std::hash<decltype(id.typeId)>()(id.typeId));
		}
	};
}

namespace Jimara {
#ifndef USE_REFACTORED_SCENE
namespace Refactor_TMP_Namespace {
#endif
	namespace SceneCachedInstances {
		/// <summary> Type of the object that can be cached 'globally' by type and SceneContext </summary>
		typedef ObjectCache<InstanceId>::StoredObject InstanceType;

		/// <summary>
		/// Retrieves the cached instance of some type, tied to a SceneContext
		/// Notes:
		///		0. This function, naturally, returns any value it finds in the global cache that if of the correct type; this means that createFn mey be ignored;
		///		1. Taking above into consideration, it is highly recommended that createFn is always the same for each TypeId, does not rely on any kind of an external state
		///		and always behaves the same;
		///		2. Ideal version of createFn would be something that just creates a new instance of a concrete class and passes SceneContext to it's constructor 
		///		without any further examination...
		/// </summary>
		/// <param name="instanceId"> SceneContext and TypeId </param>
		/// <param name="createFn"> Function, that will return some InstanceType object that is derived from instanceId.typeId (ignored, if the key already has a value) </param>
		/// <returns> Global instance </returns>
		Reference<InstanceType> GetObjectInstance(const InstanceId& instanceId, Reference<InstanceType>(*createFn)(Scene::LogicContext*));
	}

	/// <summary>
	/// Scene-wide collection of objects
	/// </summary>
	/// <typeparam name="Type"> 
	/// Type of the objects, stored in this collection
	/// Notes: 
	///		0. The collection is designed to flush changes on some scene lifecycle events, but by design, is not restricted to any one of them;
	///		1. To specify on which event this collection flushes it's changes, there has to be a static Type::OnFlushSceneObjectCollections(SceneContext*) 
	///		function that will return the correct callback;
	///		2. Type::OnFlushSceneObjectCollections(SceneContext*) should always return the same event for the same scene context, or, otherwise, 
	///		there will be a crash or two if and when the collection goes out of scope;
	///		3. The collection holds instances with their 'owners' to make sure, nobody can erase the objects from it, unless one knows the owner as well;
	///		4. Objects and Object owners are allowed to hold strong references to the collection instance, since the stored data will 
	///		automagically be erased as soon as the scene goes out of scope.
	/// </typeparam>
	template<typename Type>
	class SceneObjectCollection final : public virtual SceneCachedInstances::InstanceType {
	public:
		/// <summary>
		/// Gets instance for the LogicContext
		/// </summary>
		/// <param name="context"> Scene::LogicContext </param>
		/// <returns> Instance, tied to the context </returns>
		inline static Reference<SceneObjectCollection> GetInstance(Scene::LogicContext* context) {
			if (context == nullptr) return nullptr;
			Reference<SceneCachedInstances::InstanceType>(*createFn)(Scene::LogicContext*) = [](Scene::LogicContext* ctx) -> Reference<SceneCachedInstances::InstanceType> {
				Reference<SceneObjectCollection> newInstance = new SceneObjectCollection(ctx);
				newInstance->ReleaseRef();
				Type::OnFlushSceneObjectCollections(ctx) += Callback<>(&SceneObjectCollection::Flush, newInstance.operator->());
				return newInstance;
			};
			return SceneCachedInstances::GetObjectInstance(SceneCachedInstances::InstanceId(context, TypeId::Of<SceneObjectCollection<Type>>()), createFn);
		}

		/// <summary> Virtual destructor </summary>
		inline virtual ~SceneObjectCollection() {
			Type::OnFlushSceneObjectCollections(m_context.operator->()) -= Callback<>(&SceneObjectCollection::Flush, this);
		}

		/// <summary> Scene::LogicContext, this collection belongs to </summary>
		inline Scene::LogicContext* Context()const { return m_context; }

		/// <summary>
		/// Owner of the stored item
		/// Note: 
		///		Collection stores items alongside the owners to make sure 
		///		nobody can erase the item from the collection without first getting aquinted with the owner.
		/// </summary>
		class ItemOwner : public virtual Object {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="item"> Item to 'own' </param>
			inline ItemOwner(Type* item) : m_item(item) {}

			/// <summary> Item that can be stored inside the SceneObjectCollection </summary>
			inline Type* Item()const { return m_item; }

		private:
			// Item
			const Reference<Type> m_item;
		};

		/// <summary>
		/// Adds owner and it's item to the collection
		/// Note: Listeners will be notified on Type::OnFlushSceneObjectCollections(context) event
		/// </summary>
		/// <param name="item"> Item owner </param>
		inline void Add(ItemOwner* item) {
			Reference<Data> data = GetData();
			std::unique_lock<std::mutex> lock(data->ownerLock);
			data->ownerSet.ScheduleAdd(item);
			m_context->StoreDataObject(data);
		}

		/// <summary>
		/// Removes owner and it's item from the collection
		/// Note: Listeners will be notified on Type::OnFlushSceneObjectCollections(context) event
		/// </summary>
		/// <param name="item"> Item owner </param>
		inline void Remove(ItemOwner* item) {
			Reference<Data> data = GetData();
			std::unique_lock<std::mutex> lock(data->ownerLock);
			data->ownerSet.ScheduleRemove(item);
			m_context->StoreDataObject(data);
		}

		/// <summary>
		/// Notifies when some items get added to the collection
		/// Notes:
		///		0. Listeners will be notified on Type::OnFlushSceneObjectCollections(context) event;
		///		1. First argument is the list of object pointers, second is the number of pointers added.
		/// </summary>
		inline Event<Type* const*, size_t>& OnAdded()const { return m_onAdded; }

		/// <summary>
		/// Notifies when some items get removed from the collection
		/// Notes:
		///		0. Listeners will be notified on Type::OnFlushSceneObjectCollections(context) event;
		///		1. First argument is the list of object pointers, second is the number of pointers removed.
		/// </summary>
		inline Event<Type* const*, size_t>& OnRemoved()const { return m_onRemoved; }

		/// <summary>
		/// Reports all items currently stored inside the collection
		/// Note: Content and behaviour is updates on Type::OnFlushSceneObjectCollections(context) event exclusively.
		/// </summary>
		/// <typeparam name="ReportObjectFn"> Any function/callback, that can receive Type*-s as arguments </typeparam>
		/// <param name="reportObject"> Each stored item will be reported through this callback </param>
		template<typename ReportObjectFn>
		inline void GetAll(const ReportObjectFn& reportObject)const {
			Reference<Data> data = GetData();
			std::unique_lock<std::shared_mutex> storedObjectsLock(data->storedObjectsLock);
			for (typename Data::Objects::const_iterator it = data->storedObjects.begin(); it != data->storedObjects.end(); ++it)
				reportObject(it->first.operator->());
		}

		/// <summary> Invoked each time the collection gets updated (after OnAdded() and OnRemoved(), even if no change occures) </summary>
		inline Event<>& OnFlushed()const { return m_onFlushed; }

	private:
		// Scene::LogicContext, this collection belongs to
		const Reference<Scene::LogicContext> m_context;

		// Notifies about added objects
		mutable EventInstance<Type* const*, size_t> m_onAdded;

		// Notifies about removed objects
		mutable EventInstance<Type* const*, size_t> m_onRemoved;

		// Invoked each time the collection gets updated
		mutable EventInstance<> m_onFlushed;

		// Scene-wide data (erased only when empty; otherwise stored with StoreDataObject())
		struct Data;
		mutable SpinLock m_dataLock;
		mutable Data* m_data = nullptr;

		// Constructor
		inline SceneObjectCollection(Scene::LogicContext* context) : m_context(context) {
			assert(m_context != nullptr);
		}

		// Flushes Add/Remove calls
		inline void Flush() {
			Reference<SceneObjectCollection> self(this);
			Reference<Data> data = GetData();
			std::unique_lock<std::mutex> flushLock(data->flushLock);

			// Flush added/removed object collection:
			std::vector<Reference<Type>>& addedBuffer = data->addedBuffer;
			std::vector<Reference<Type>>& removedBuffer = data->removedBuffer;
			{
				addedBuffer.clear();
				removedBuffer.clear();
				std::unique_lock<std::mutex> ownerLock(data->ownerLock);

				// Flush data:
				data->ownerSet.Flush(
					[&](const Reference<ItemOwner>* removed, size_t count) {
						for (size_t i = 0; i < count; i++)
							removedBuffer.push_back(removed[i]->Item());
					},
					[&](const Reference<ItemOwner>* added, size_t count) {
						for (size_t i = 0; i < count; i++)
							addedBuffer.push_back(added[i]->Item());
					});

				// If data is empty, there's no need to hold the context-wide reference any more...
				if (data->ownerSet.Size() <= 0)
					m_context->EraseDataObject(data);
			}

			// Find added/removed objects:
			std::vector<Type*>& addedObjects = data->addedObjects;
			std::vector<Type*>& removedObjects = data->removedObjects;
			{
				addedObjects.clear();
				removedObjects.clear();
				std::unique_lock<std::shared_mutex> storedObjectsLock(data->storedObjectsLock);

				// Record new objects:
				for (size_t i = 0; i < addedBuffer.size(); i++) {
					Type* item = addedBuffer[i];
					if (item == nullptr) return;
					typename Data::Objects::iterator it = data->storedObjects.find(item);
					if (it == data->storedObjects.end()) {
						data->storedObjects[item] = 1;
						addedObjects.push_back(item);
					}
					else it->second++;
				}

				// See which objects got lost:
				for (size_t i = 0; i < removedBuffer.size(); i++) {
					Type* item = removedBuffer[i];
					if (item == nullptr) return;
					typename Data::Objects::iterator it = data->storedObjects.find(item);
					if (it == data->storedObjects.end()) continue;
					else if (it->second == 1) {
						data->storedObjects.erase(it);
						removedObjects.push_back(item);
					}
					else it->second--;
				}

				// Erase 'added' objects that were actually lost after removedBuffer got examined
				{
					size_t i = 0;
					while (i < addedObjects.size()) {
						Type* item = addedObjects[i];
						if (item == nullptr) return;
						typename Data::Objects::iterator it = data->storedObjects.find(item);
						if (it == data->storedObjects.end()) {
							addedObjects[i] = addedObjects.back();
							addedObjects.pop_back();
						}
						else i++;
					}
				}
			}

			// Notify changes and clear buffers:
			{
				m_onRemoved(removedObjects.data(), removedObjects.size());
				m_onAdded(addedObjects.data(), addedObjects.size());
				m_onFlushed();
				addedObjects.clear();
				removedObjects.clear();
				addedBuffer.clear();
				removedBuffer.clear();
			}
		}

		// Scene-wide data 
		struct Data : public virtual Object {
			const Reference<const SceneObjectCollection> dataOwner;
			std::mutex ownerLock;
			DelayedObjectSet<ItemOwner> ownerSet;

			std::shared_mutex storedObjectsLock;
			typedef std::unordered_map<Reference<Type>, size_t> Objects;
			Objects storedObjects;

			std::mutex flushLock;
			std::vector<Reference<Type>> addedBuffer;
			std::vector<Reference<Type>> removedBuffer;
			std::vector<Type*> addedObjects;
			std::vector<Type*> removedObjects;

			inline Data(const SceneObjectCollection* owner) : dataOwner(owner) {}
			inline virtual void OnOutOfScope()const final override {
				std::unique_lock<SpinLock> lock(dataOwner->m_dataLock);
				if (RefCount() > 0) return;
				dataOwner->m_data = nullptr;
				Object::OnOutOfScope();
			}
		};

		// Gets scene-wide data
		inline Reference<Data> GetData()const {
			std::unique_lock<SpinLock> lock(m_dataLock);
			Reference<Data> result(m_data);
			if (result == nullptr) {
				result = Object::Instantiate<Data>(this);
				m_data = result;
			}
			return result;
		}
	};
#ifndef USE_REFACTORED_SCENE
}
#endif
}
#pragma once
#include "TypeRegistartion.h"
#include <unordered_set>


namespace Jimara {
	/// <summary>
	/// An "Object Factory" that can create arbitrary derived classes without the user knowing about the concrete type
	/// <para/> Note: If you wish your factory to be a part of ObjectFactory::Set, you must register your class and report it's factory as an attribute.
	/// </summary>
	/// <typeparam name="ObjectType"> Base class </typeparam>
	/// <typeparam name="...ArgTypes"> Constructor arguments </typeparam>
	template<typename ObjectType, typename... ArgTypes>
	class JIMARA_API ObjectFactory : public virtual Object {
	public:
		/// <summary>
		/// Copy-constructor
		/// </summary>
		/// <param name="src"> Factory to copy </param>
		inline ObjectFactory(const ObjectFactory& src) : ObjectFactory(src.m_createFn, src.m_typeId, src.m_name, src.m_path, src.m_hint) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~ObjectFactory() {}

		/// <summary>
		/// Creates an ObjectFactory instance for the given type
		/// </summary>
		/// <typeparam name="ConcreteType"> Concrete object type to instantiate </typeparam>
		/// <param name="itemName"> 'Default name' of the item (when applicable) </param>
		/// <param name="menuPath"> 'Context menu path' of the factory (when applicable; primarily used by the Editor application) </param>
		/// <param name="hint"> Arbitrary information about the object type (primarily used by the Editor application to display hints) </param>
		/// <returns> A new instance of an ObjectFactory </returns>
		template<typename ConcreteType>
		inline static Reference<ObjectFactory> Create(
			const std::string_view& itemName = TypeId::Of<ConcreteType>().Name(),
			const std::string_view& menuPath = TypeId::Of<ConcreteType>().Name(),
			const std::string_view& hint = TypeId::Of<ConcreteType>().Name());

		/// <summary>
		/// Creates a concreate instance of ObjectType, based on whatever type the ObjectFactory::Create got as it's template argument
		/// </summary>
		/// <param name="...args"> Constructor arguments </param>
		/// <returns> New instance of ObjectType (concrete type will be InstanceType()) </returns>
		inline Reference<ObjectType> CreateInstance(ArgTypes... args)const { return m_createFn(args...); }

		/// <summary> Type of the concrete instance CreateInstance() method will create </summary>
		inline TypeId InstanceType()const { return m_typeId; }

		/// <summary> 'Default name' of the item (when applicable) </summary>
		inline const std::string& ItemName()const { return m_name; }

		/// <summary> 'Context menu path' of the factory (when applicable; primarily used by the Editor application) </summary>
		inline const std::string& MenuPath()const { return m_path; }

		/// <summary> Arbitrary information about the object type (primarily used by the Editor application to display hints) </summary>
		inline const std::string& Hint()const { return m_hint; }

		/// <summary>
		/// A collection of ObjectFactory items
		/// </summary>
		class JIMARA_API Set : public virtual Object {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="factories"> List of factories to construct the set from </param>
			/// <param name="count"> Number of factories within the list </param>
			inline Set(const Reference<const ObjectFactory>* factories, size_t count);

			/// <summary> Virtual destructor </summary>
			inline virtual ~Set() {}

			/// <summary> Number of factories within the set </summary>
			inline size_t Size()const { return m_factories.size(); }

			/// <summary>
			/// Gets factory by index
			/// </summary>
			/// <param name="index"> Index of the factory </param>
			/// <returns> Factory </returns>
			inline const ObjectFactory* At(size_t index)const { return m_factories[index]; }

			/// <summary>
			/// Gets factory by index
			/// </summary>
			/// <param name="index"> Index of the factory </param>
			/// <returns> Factory </returns>
			inline const ObjectFactory* operator[](size_t index)const { return m_factories[index]; }

			/// <summary> If IndexOf() query returns this, it means the factory is not in the set </summary>
			inline static constexpr size_t InvalidIndex() { return ~size_t(0u); }

			/// <summary>
			/// Finds index of a given factory or InvalidIndex if the factory is not in the set
			/// </summary>
			/// <param name="factory"> Factory to search for </param>
			/// <returns> Factory index or InvalidIndex </returns>
			inline size_t IndexOf(const ObjectFactory* factory)const { return FindIndex<const ObjectFactory*>(m_indexByFactory, factory); }

			/// <summary>
			/// Finds factory based on the concreate type id
			/// </summary>
			/// <param name="instanceType"> Concreate/InstanceType type </param>
			/// <returns> Factory (nullptr if not found) </returns>
			inline const ObjectFactory* FindFactory(const TypeId& instanceType)const { return FindFactory(instanceType.TypeIndex()); }

			/// <summary>
			/// Finds factory based on the concreate type index
			/// </summary>
			/// <param name="instanceType"> Concreate/InstanceType type </param>
			/// <returns> Factory (nullptr if not found) </returns>
			inline const ObjectFactory* FindFactory(const std::type_index& instanceTypeIndex)const { return Find<std::type_index>(m_indexByType, instanceTypeIndex); }

			/// <summary>
			/// Finds factory based on the concreate type name
			/// </summary>
			/// <param name="instanceType"> Concreate/InstanceType type name </param>
			/// <returns> Factory (nullptr if not found) </returns>
			inline const ObjectFactory* FindFactory(const std::string_view& instanceTypeName)const { return Find<std::string_view>(m_indexByTypeName, instanceTypeName); }

			/// <summary>
			/// Finds a factory that can construct items of the same concreate type
			/// </summary>
			/// <param name="instance"> Item </param>
			/// <returns> Factory (nullptr if not found) </returns>
			inline const ObjectFactory* FindFactory(const ObjectType* instance)const { return (instance == nullptr) ? nullptr : FindFactory(typeid(*instance)); }

			/// <summary>
			/// Finds a factory that can construct items of the same concreate type
			/// </summary>
			/// <param name="instance"> Item </param>
			/// <returns> Factory (nullptr if not found) </returns>
			inline const ObjectFactory* FindFactory(const ObjectType& instance)const { return FindFactory(&instance); }

		private:
			// List of factories
			std::vector<Reference<const ObjectFactory>> m_factories;

			// Index by concreate type index
			std::unordered_map<std::type_index, size_t> m_indexByType;

			// Index by concreate type name
			std::unordered_map<std::string_view, size_t> m_indexByTypeName;

			// Index by factory
			std::unordered_map<const ObjectFactory*, size_t> m_indexByFactory;

			// Index search function
			template<typename KeyType>
			inline static size_t FindIndex(const std::unordered_map<KeyType, size_t>& index, const KeyType& key) {
				const typename std::unordered_map<KeyType, size_t>::const_iterator it = index.find(key);
				return (it == index.end()) ? InvalidIndex() : it->second;
			}

			// Factory search function
			template<typename KeyType>
			inline const ObjectFactory* Find(const std::unordered_map<KeyType, size_t>& index, const KeyType& key)const {
				const size_t id = FindIndex<KeyType>(index, key);
				return (id < m_factories.size()) ? m_factories[id] : nullptr;
			}
		};

		/// <summary> Set of all factories, reported as registered type attributes </summary>
		inline static Reference<Set> All();



	private:
		// Constructor wrapper
		typedef Reference<ObjectType>(*CreateFn)(ArgTypes...);
		const CreateFn m_createFn;

		// Concrete type id
		const TypeId m_typeId;

		// "Instance Name"
		const std::string m_name;

		// "Context menu path"
		const std::string m_path;

		// "Type hint"
		const std::string m_hint;

		// Actual constructor needs to be private!
		inline ObjectFactory(CreateFn createFn, const TypeId& typeId, const std::string_view& name, const std::string_view& path, const std::string_view& hint)
			: m_createFn(createFn), m_typeId(typeId), m_name(name), m_path(path), m_hint(hint) {}
	};


	template<typename ObjectType, typename... ArgTypes>
	template<typename ConcreteType>
	inline Reference<ObjectFactory<ObjectType, ArgTypes...>> ObjectFactory<ObjectType, ArgTypes...>::Create(
		const std::string_view& itemName, const std::string_view& menuPath, const std::string_view& hint) {

		static const CreateFn createFn = [](ArgTypes... args) -> Reference<ObjectType> {
			const Reference<ConcreteType> instance = new ConcreteType(args...);
			instance->ReleaseRef();
			return Reference<ObjectType>(instance.operator->());
		};

		const Reference<ObjectFactory> instance = new ObjectFactory(createFn, TypeId::Of<ConcreteType>(), itemName, menuPath, hint);
		instance->ReleaseRef();
		return instance;
	}

	template<typename ObjectType, typename... ArgTypes>
	inline ObjectFactory<ObjectType, ArgTypes...>::Set::Set(const Reference<const ObjectFactory>* factories, size_t count) {
		std::unordered_set<const ObjectFactory*> factorySet;
		const Reference<const ObjectFactory>* ptr = factories;
		const Reference<const ObjectFactory>* const end = ptr + count;
		while (ptr < end) {
			const ObjectFactory* factory = *ptr;
			ptr++;
			if (factory == nullptr) continue;
			if (factorySet.find(factory) != factorySet.end()) continue;
			factorySet.insert(factory);
			m_indexByType[factory->InstanceType().TypeIndex()] = m_factories.size();
			m_indexByTypeName[factory->InstanceType().Name()] = m_factories.size();
			m_indexByFactory[factory] = m_factories.size();
			m_factories.push_back(factory);
		}
	}

	template<typename ObjectType, typename... ArgTypes>
	inline Reference<typename ObjectFactory<ObjectType, ArgTypes...>::Set> ObjectFactory<ObjectType, ArgTypes...>::All() {
		static SpinLock INSTANCE_LOCK;
		static std::mutex CREATION_LOCK;
		static Reference<Set> INSTANCE;

		struct RegistrySubscription : public virtual Object {
			inline static void OnRegisteredTypeSetChanged() {
				std::unique_lock<std::mutex> creationLock(CREATION_LOCK);
				std::unique_lock<std::mutex> lock(INSTANCE_LOCK);
				INSTANCE = nullptr;
			}
			inline RegistrySubscription() {
				TypeId::OnRegisteredTypeSetChanged() += Callback<>::FromCall(RegistrySubscription::OnRegisteredTypeSetChanged);
			}
			inline virtual ~RegistrySubscription() {
				TypeId::OnRegisteredTypeSetChanged() -= Callback<>::FromCall(RegistrySubscription::OnRegisteredTypeSetChanged);
			}
		};
		static Reference<RegistrySubscription> REGISTRY_SUBSCRIPTION;

		static bool SUBSCRIBED_TO_REGISTRY = false;

		static auto getInstance = []() {
			std::unique_lock<SpinLock> lock(INSTANCE_LOCK);
			Reference<Set> rv = INSTANCE;
			return rv;
		};
		Reference<Set> instance = getInstance();
		if (instance != nullptr) return instance;

		std::unique_lock<std::mutex> creationLock(CREATION_LOCK);
		
		if (REGISTRY_SUBSCRIPTION == nullptr)
			REGISTRY_SUBSCRIPTION = Object::Instantiate<RegistrySubscription>();

		instance = getInstance();
		if (instance != nullptr) return instance;

		const Reference<RegisteredTypeSet> currentTypes = RegisteredTypeSet::Current();
		std::vector<Reference<const ObjectFactory>> factories;
		for (size_t i = 0; i < currentTypes->Size(); i++) {
			auto checkAttribute = [&](const Object* attribute) {
				const ObjectFactory* factory = dynamic_cast<const ObjectFactory*>(attribute);
				if (factory != nullptr) factories.push_back(factory);
			};
			currentTypes->At(i).GetAttributes(Callback<const Object*>::FromCall(&checkAttribute));
		}

		instance = Object::Instantiate<Set>(factories.data(), factories.size());
		{
			std::unique_lock<SpinLock> lock(INSTANCE_LOCK);
			INSTANCE = instance;
		}
		return instance;
	}
}

#pragma once
#include "../Environment/Scene/Scene.h"


namespace Jimara {
	/// <summary>
	/// Registry can be used to 'automagically' resolve references to the objects that can not be defined inside the editor,
	/// or need to be updated on the fly
	/// <para/> Notes: 
	/// <para/>		0. Generally speaking, you can think of a Registry as a thread-safe multi-map of objects, where for each key,
	///				you have a set of Entries you can add to by creating an Entry and read from with a Reader.
	/// <para/>		1. There can be multiple types of Registries, several of which are as follows:
	/// <para/>			. Global Registry: A single instance of a Registry, that is globally accessed using Registry::Global();
	/// <para/>			. Context-Wide Registry: Shared registry for entire scene context, but not shared between contexts, accessed using Registry::ContextWide(ctx);
	/// <para/>			. ComponentRegistry: A simple scene component that inherits Registry and can freely exist in hierarchy;
	/// <para/>			. Any other custom instances can be created on the fly if so desired;
	/// </summary>
	class JIMARA_API Registry : public virtual Object {
	private:
		// Some of the private underlying details of the implementation reside in here...
		struct Helpers;

	public:
		/// <summary> Constructor </summary>
		Registry();

		/// <summary> Virtual Destructor </summary>
		virtual ~Registry();

		/// <summary> Global singleton instance of a Registry </summary>
		static Registry* Global();

		/// <summary>
		/// Context-wide registry to be shared within a scene context
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> Shared Instance of a context-wide registry </returns>
		static Reference<Registry> ContextWide(SceneContext* context);

		/// <summary> Registry entry (while in scope, this structure makes sure an object is stored in a registry) </summary>
		class JIMARA_API Entry;

		/// <summary> Atomic reader for registry entries </summary>
		class JIMARA_API Reader;

		/// <summary>
		/// Container of all live entries for each registry key
		/// </summary>
		class JIMARA_API Entries : public virtual Object {
		public:
			/// <summary> Virtual destructor </summary>
			virtual ~Entries();

			/// <summary> Event, invoked each time the content of the entries changes </summary>
			inline Jimara::Event<Entries*>& OnDirty()const { return m_onDirty; }

		private:
			// Lock for entries
			mutable std::shared_mutex m_lock;
			
			// Entry data
			struct EntryInfo {
				std::unordered_set<Entry*> registryEntries;
				size_t index = ~size_t(0u);
			};
			std::map<Reference<Object>, EntryInfo> m_entries;

			// Store object list
			Jimara::Stacktor<Object*, 1u> m_objects;

			// OnDirty() event
			mutable Jimara::EventInstance<Entries*> m_onDirty;

			// Reader, Helpers and Registry need direct access to the internals
			friend class Reader;
			friend struct Helpers;
			friend class Registry;

			// Constructor is private
			Entries();
		};

		/// <summary>
		/// Retrieves entry set for given key
		/// </summary>
		/// <param name="key"> Unique identifier for the entry set </param>
		/// <returns> Entries for the key </returns>
		Reference<Entries> GetEntries(const Object* key);

		/// <summary>
		/// Retrieves entry set for given key
		/// </summary>
		/// <param name="key"> Unique identifier for the entry set </param>
		/// <returns> Entries for the key </returns>
		Reference<Entries> GetEntries(const std::string& key);

	private:
		// Underlying data
		const Reference<Object> m_entryCache;
	};



	/// <summary>
	/// Registry entry (while in scope, this structure makes sure an object is stored in a registry)
	/// </summary>
	class JIMARA_API Registry::Entry final {
	public:
		/// <summary> Constructor </summary>
		Entry();

		/// <summary>
		/// Constructor
		/// <para/> Stores given item inside the entry set of the given key
		/// </summary>
		/// <param name="registry"> Registry </param>
		/// <param name="key"> Entry set key </param>
		/// <param name="item"> Stored object reference </param>
		Entry(Registry* registry, const Object* key, Object* item);

		/// <summary>
		/// Constructor
		/// <para/> Stores given item inside the entry set of the given key
		/// </summary>
		/// <param name="registry"> Registry </param>
		/// <param name="key"> Entry set key </param>
		/// <param name="item"> Stored object reference </param>
		Entry(Registry* registry, const std::string& key, Object* item);

		/// <summary>
		/// Constructor
		/// <para/> Stores given item inside the entry set
		/// </summary>
		/// <param name="entries"> Entry collection </param>
		/// <param name="item"> Stored object reference </param>
		Entry(const Reference<Entries>& entries, Object* item);

		/// <summary>
		/// Copy-Constructor
		/// </summary>
		/// <param name="other"> Entry to copy </param>
		Entry(const Entry& other);

		/// <summary>
		/// Move-Constuctor
		/// </summary>
		/// <param name="other"> Entry to move </param>
		Entry(Entry&& other)noexcept;

		/// <summary> Destructor (removes entry record) </summary>
		~Entry();

		/// <summary>
		/// Copy-Assignment
		/// </summary>
		/// <param name="other"> Entry to duplicate </param>
		/// <returns> Self </returns>
		Entry& operator=(const Entry& other);
		
		/// <summary>
		/// Move-Assignment
		/// </summary>
		/// <param name="other"> Entry to move </param>
		/// <returns> Self </returns>
		Entry& operator=(Entry&& other)noexcept;

	private:
		// State lock
		mutable Jimara::SpinLock m_lock;

		// Stored object reference
		Reference<Object> m_storedObject;

		// Entry set
		Reference<Entries> m_entries;

		// Registry and helpers need access to internals
		friend class Registry;
		friend struct Helpers;
	};



	/// <summary>
	/// Atomic reader for registry entries
	/// <para/> Note: While alive, it is illegal to add/remove entries to/from the set from the same thread!
	/// </summary>
	class JIMARA_API Registry::Reader final {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="registry"> Registry to read from (if nullptr, Global registry will be used) </param>
		/// <param name="key"> Registry key </param>
		Reader(Registry* registry, const Object* key);

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="registry"> Registry to read from (if nullptr, Global registry will be used) </param>
		/// <param name="key"> Registry key </param>
		Reader(Registry* registry, const std::string& key);

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="entries"> Entry set </param>
		Reader(const Reference<const Entries>& entries);

		/// <summary> Destructor </summary>
		~Reader();

		/// <summary> Number of elements within the entry set </summary>
		size_t ItemCount()const;

		/// <summary>
		/// Stored element from entry set by index
		/// </summary>
		/// <param name="index"> Element index ([0 - ItemCount()) range) </param>
		/// <returns> Stored object </returns>
		Object* Item(size_t index)const;

	private:
		// Entries
		const Reference<const Entries> m_entries;
		
		// Read-lock for entries
		const std::shared_lock<std::shared_mutex> m_lock;
		
		// Number of stored elements
		size_t m_itemCount = 0u;

		// Copy/Move is prohibited
		inline Reader(const Reader&) = delete;
		inline Reader& operator=(const Reader&) = delete;
		inline Reader(Reader&&) = delete;
		inline Reader& operator=(Reader&&) = delete;
	};


	/// <summary> Expose parent type of Registry </summary>
	template<> inline void TypeIdDetails::GetParentTypesOf<Registry>(const Callback<TypeId>& report) {
		report(TypeId::Of<Object>());
	}
}

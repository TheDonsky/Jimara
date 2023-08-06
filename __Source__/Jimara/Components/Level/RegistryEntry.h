#pragma once
#include "ComponentRegistry.h"


namespace Jimara {
	/// <summary> Let the system know the type exists </summary>
	JIMARA_REGISTER_TYPE(Jimara::RegistryEntry);

	/// <summary>
	/// A component that stores and maintains a single registry entry
	/// <para/> Entry is stored only while the component is active and enabled
	/// </summary>
	class JIMARA_API RegistryEntry : public virtual Component {
	public:
		/// <summary> Type of the registry to store element in </summary>
		enum class JIMARA_API RegistryType : uint8_t {
			/// <summary> Disables any storage operation </summary>
			NONE = 0u,

			/// <summary> Component will search a registry in parent hierarchy and use it if found </summary>
			PARENT = 1u,

			/// <summary> A custom user-defined Registry will be used </summary>
			CUSTOM = 2u,

			/// <summary> Scene-wide registry will be used </summary>
			SCENE_WIDE = 3u,

			/// <summary> Global registry instance will be used </summary>
			GLOBAL = 4u,
		};

		/// <summary> Enumeration attribute for RegistryType serialization </summary>
		static const Object* RegistryTypeEnumAttribute();

		/// <summary> Type of the registry key </summary>
		enum class JIMARA_API KeyType : uint8_t {
			/// <summary> Object reference will be used as the key if not nullptr </summary>
			OBJECT,

			/// <summary> String key will be used if not empty </summary>
			STRING
		};

		/// <summary> Enumeration attribute for KeyType serialization </summary>
		static const Object* KeyTypeEnumAttribute();

		/// <summary> Configuration of stored object </summary>
		enum class JIMARA_API StoredObjectType : uint8_t {
			/// <summary> Parent component will be stored in entries </summary>
			PARENT = 0u,

			/// <summary> This component will be stored in entries </summary>
			SELF = 1u,

			/// <summary> Custom component reference will be stored in entries </summary>
			CUSTOM = 2u
		};

		/// <summary> Enumeration attribute for StoredObjectType serialization </summary>
		static const Object* StoredObjectTypeEnumAttribute();

		/// <summary>
		/// Partial configuration, containing relevant information about the registry and key
		/// </summary>
		struct JIMARA_API EntrySetConfiguration {
			/// <summary> Registry settings </summary>
			struct JIMARA_API {
				/// <summary> Registry type to use </summary>
				RegistryType type = RegistryType::PARENT;

				/// <summary> [Optional] Reference to the custom registry (ignored/undefined if RegistryType is not set to CUSTOM) </summary>
				Reference<Registry> reference;
			} registry;

			/// <summary> Key configuration </summary>
			struct JIMARA_API {
				/// <summary> Key type to use </summary>
				KeyType type = KeyType::OBJECT;

				/// <summary> Object key (used if and only if KeyType is OBJECT) </summary>
				Reference<Object> object;

				/// <summary> String key (used if and only if KeyType is STRING) </summary>
				std::string string;
			} key;

			/// <summary> Serializer for EntrySetConfiguration </summary>
			struct JIMARA_API Serializer;
		};

		/// <summary> RegistryEntry Configuration </summary>
		struct JIMARA_API Configuration final : public EntrySetConfiguration {
			/// <summary> Stored object configuration </summary>
			struct JIMARA_API {
				/// <summary> Stored object type </summary>
				StoredObjectType type = StoredObjectType::PARENT;

				/// <summary> [Optional] Stored object reference (ignored/undefined if StoredObjectType is not set to CUSTOM) </summary>
				Reference<Object> reference;
			} storedObject;

			/// <summary> Serializer for Configuration </summary>
			struct JIMARA_API Serializer;
		};


		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		RegistryEntry(Component* parent, const std::string_view& name = "RegistryEntry");

		/// <summary> Virtual destructor </summary>
		virtual ~RegistryEntry();

		/// <summary> Retrieves current configuration of RegistryEntry </summary>
		Configuration GetConfiguration()const;

		/// <summary>
		/// Sets configuration of RegistryEntry
		/// </summary>
		/// <param name="settings"> Configuration settings </param>
		void Configure(const Configuration& settings);

		/// <summary>
		/// Exposes fields to serialization utilities
		/// </summary>
		/// <param name="recordElement"> Reports elements with this </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;


	protected:
		/// <summary> Stores entry in the registry if possible </summary>
		virtual void OnComponentEnabled()override;

		/// <summary> Removes entry from the registry if possible </summary>
		virtual void OnComponentDisabled()override;

	private:
		// Lock for configuration changes
		mutable std::mutex m_updateLock;

		// Current configuration
		Configuration m_configuration;

		// Components, events of which the RegistryEntry is subscribed to
		Stacktor<Reference<Component>, 4u> m_subscribedComponents;

		// Some updates happen asynchronously; this value is used to determine if there's anything scheduled or not
		std::atomic<size_t> m_scheduledRefreshCount = 0u;
		
		// Underlying entry
		Registry::Entry m_entry;

		// Private stuff resides in here...
		struct Helpers;
	};



	/// <summary> Serializer for EntrySetConfiguration </summary>
	struct JIMARA_API RegistryEntry::EntrySetConfiguration::Serializer : public virtual Serialization::SerializerList::From<EntrySetConfiguration> {
		/// <summary>
		/// Consttructor
		/// </summary>
		/// <param name="name"> Serializer name </param>
		/// <param name="hint"> Serializer hint </param>
		/// <param name="attributes"> Serializer attributes </param>
		Serializer(const std::string_view& name, const std::string_view& hint, const std::vector<Reference<const Object>>& attributes = {})
			: Serialization::ItemSerializer(name, hint, attributes) {}
		
		/// <summary> Virtual destructor </summary>
		inline virtual ~Serializer() {}
		
		/// <summary>
		/// Gives access to sub-serializers/fields
		/// </summary>
		/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
		/// <param name="targetAddr"> Serializer target object </param>
		virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, EntrySetConfiguration* target)const override;
	};

	/// <summary> Serializer for Configuration </summary>
	struct JIMARA_API RegistryEntry::Configuration::Serializer : public virtual Serialization::SerializerList::From<Configuration> {
		/// <summary>
		/// Consttructor
		/// </summary>
		/// <param name="name"> Serializer name </param>
		/// <param name="hint"> Serializer hint </param>
		/// <param name="attributes"> Serializer attributes </param>
		Serializer(const std::string_view& name, const std::string_view& hint, const std::vector<Reference<const Object>>& attributes = {})
			: Serialization::ItemSerializer(name, hint, attributes) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Serializer() {}

		/// <summary>
		/// Gives access to sub-serializers/fields
		/// </summary>
		/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
		/// <param name="targetAddr"> Serializer target object </param>
		virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, Configuration* target)const override;
	};

	// Type detail callbacks
	template<> JIMARA_API void TypeIdDetails::GetParentTypesOf<RegistryEntry>(const Callback<TypeId>& report);
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<RegistryEntry>(const Callback<const Object*>& report);
}

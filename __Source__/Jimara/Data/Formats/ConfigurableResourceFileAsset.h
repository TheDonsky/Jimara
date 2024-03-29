#pragma once
#include "../ConfigurableResource.h"
#include "../Serialization/Helpers/SerializeToJson.h"


namespace Jimara {
	class ConfigurableResourceFileAsset;

	// Registration
	JIMARA_REGISTER_TYPE(Jimara::ConfigurableResourceFileAsset);

	// TypeId detail callbacks
	template<> JIMARA_API void TypeIdDetails::OnRegisterType<ConfigurableResourceFileAsset>();
	template<> JIMARA_API void TypeIdDetails::OnUnregisterType<ConfigurableResourceFileAsset>();
	template<> struct JIMARA_API TypeIdDetails::TypeDetails<ConfigurableResourceFileAsset> {
	private:
		static void OnRegisterType();
		static void OnUnregisterType();

		friend void TypeIdDetails::OnRegisterType<ConfigurableResourceFileAsset>();
		friend void TypeIdDetails::OnUnregisterType<ConfigurableResourceFileAsset>();
	};
	template<> inline void TypeIdDetails::GetParentTypesOf<ConfigurableResourceFileAsset>(const Callback<TypeId>& report) {
		report(TypeId::Of<ModifiableAsset>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<ConfigurableResourceFileAsset>(const Callback<const Object*>&) {}

#pragma warning(disable: 4250)
	/// <summary>
	/// Configurable resource asset within the file system database
	/// </summary>
	class JIMARA_API ConfigurableResourceFileAsset : public virtual ModifiableAsset {
	public:
		/// <summary> True, if the resource, once loaded, can have any recursive external dependencies </summary>
		inline virtual bool HasRecursiveDependencies()const final override { return true; }

		/// <summary> Type of the resource, this asset can load </summary>
		virtual TypeId ResourceType()const final override;

		/// <summary>
		/// Stores resource data to the file
		/// </summary>
		virtual void StoreResource()final override;

	protected:
		/// <summary>
		/// Loads resource
		/// </summary>
		/// <returns> Resource </returns>
		virtual Reference<Resource> LoadResourceObject()final override;

		/// <summary>
		/// Unloads resource
		/// </summary>
		/// <param name="resource"> Resource to release </param>
		inline virtual void UnloadResourceObject(Reference<Resource> resource)final override {}

		/// <summary>
		///Refreshes/reloads all 'external' dependencies, thus making sure the Resource is up to date with the Asset Database.
		/// </summary>
		/// <param name="resource"> Resource, previously loaded with LoadItem() </param>
		virtual void RefreshExternalDependencies(Resource* resource)final override;

	public:
		/// <summary>
		/// Serializes resource into json
		/// </summary>
		/// <param name="resource"> Resource to serialize </param>
		/// <param name="log"> Logger for error reporting </param>
		/// <param name="error"> If error occures, this flag will be set </param>
		/// <returns> Serialized data </returns>
		static nlohmann::json SerializeToJson(ConfigurableResource::SerializableInstance* resource, OS::Logger* log, bool& error);

		/// <summary>
		/// Extracts resource data from a json
		/// </summary>
		/// <param name="resource"> Resource to store data into </param>
		/// <param name="database"> Database for external references </param>
		/// <param name="log"> Logger for error reporting </param>
		/// <param name="serializedData"> Serialized data [possibly generated by previous SerializeToJson() call] </param>
		/// <returns> True, if no error occures </returns>
		static bool DeserializeFromJson(ConfigurableResource::SerializableInstance* resource, AssetDatabase* database, OS::Logger* log, const nlohmann::json& serializedData);

		/// <summary> ConfigurableResource file extension </summary>
		static const OS::Path& Extension();

	private:
		// Internal asset importer class
		class Importer;

		// Resource type name
		const std::string m_typeName;

		// Lock for importer reference
		SpinLock m_importerLock;

		// Importer reference (Alive only while the FileSystemDB is alive and file exists; beyond that, Load/Store operations will fail miserably)
		Importer* m_importer;

		// Constructor
		ConfigurableResourceFileAsset(const GUID& guid, Importer* importer, const std::string_view& typeName);

		// Type id details need access to internals
		friend struct TypeIdDetails::TypeDetails<ConfigurableResourceFileAsset>;
	};
#pragma warning(default: 4250)
}

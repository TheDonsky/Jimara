#pragma once
#include "../Material.h"
#include "../Serialization/Helpers/SerializeToJson.h"


namespace Jimara {
	class MaterialFileAsset;

	// Registration
	JIMARA_REGISTER_TYPE(Jimara::MaterialFileAsset);

	// TypeId detail callbacks
	template<> void TypeIdDetails::OnRegisterType<MaterialFileAsset>();
	template<> void TypeIdDetails::OnUnregisterType<MaterialFileAsset>();
	template<> struct TypeIdDetails::TypeDetails<MaterialFileAsset> {
	private:
		static void OnRegisterType();
		static void OnUnregisterType();

		friend void TypeIdDetails::OnRegisterType<MaterialFileAsset>();
		friend void TypeIdDetails::OnUnregisterType<MaterialFileAsset>();
	};
	template<> inline void TypeIdDetails::GetParentTypesOf<MaterialFileAsset>(const Callback<TypeId>& report) {
		report(TypeId::Of<ModifiableAsset::Of<Material>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<MaterialFileAsset>(const Callback<const Object*>&) {}

#pragma warning(disable: 4250)
	/// <summary>
	/// File system asset for scenes
	/// </summary>
	class MaterialFileAsset : public virtual ModifiableAsset::Of<Material> {
	protected:
		/// <summary>
		/// Loads material
		/// </summary>
		/// <returns> Heirarchy spowner </returns>
		virtual Reference<Material> LoadItem() override;

		/// <summary>
		/// Stores material data to the file
		/// </summary>
		/// <param name="resource"> Resource, that has previously been loaded with LoadItem() call and now has been changed (alegedly) </param>
		virtual void Store(Material* resource) override;

	public:
		/// <summary>
		/// Serializes Material into json
		/// </summary>
		/// <param name="material"> Material to serialize </param>
		/// <param name="log"> Logger for error reporting </param>
		/// <param name="error"> If error occures, this flag will be set </param>
		/// <returns> Serialized data </returns>
		static nlohmann::json SerializeToJson(Material* material, OS::Logger* log, bool& error);

		/// <summary>
		/// Extracts material data from a json
		/// </summary>
		/// <param name="material"> Material to store data into </param>
		/// <param name="database"> Database for external references </param>
		/// <param name="log"> Logger for error reporting </param>
		/// <param name="serializedData"> Serialized data [possibly generated by previous SerializeMaterial() call] </param>
		/// <returns> True, if no error occures </returns>
		static bool DeserializeFromJson(Material* material, AssetDatabase* database, OS::Logger* log, const nlohmann::json& serializedData);

		/// <summary> Material file extension </summary>
		static const OS::Path& Extension();

	private:
		// Internal asset importer class
		class Importer;

		// Lock for importer reference
		SpinLock m_importerLock;

		// Importer reference (Alive only while the FileSystemDB is alive and file exists; beyond that, Load/Store operations will fail miserably)
		Importer* m_importer;

		// Constructor
		MaterialFileAsset(const GUID& guid, Importer* importer);

		// Type id details need access to internals
		friend struct TypeIdDetails::TypeDetails<MaterialFileAsset>;
	};
#pragma warning(default: 4250)
}


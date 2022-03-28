#include "MaterialFileAsset.h"
#include "../AssetDatabase/FileSystemDatabase/FileSystemDatabase.h"
#include "../Serialization/Helpers/SerializeToJson.h"
#include <fstream>


namespace Jimara {
	namespace {
		inline static bool LoadMaterialFileJson(const OS::Path& path, OS::Logger* log, nlohmann::json& json) {
			const Reference<OS::MMappedFile> memoryMapping = OS::MMappedFile::Create(path, log);
			if (memoryMapping == nullptr) {
				log->Error("MaterialFileAsset::LoadMaterialFileJson - Failed to map file: \"", path, "\"!");
				return false;
			}
			try {
				MemoryBlock block(*memoryMapping);
				json = nlohmann::json::parse(std::string_view(reinterpret_cast<const char*>(block.Data()), block.Size()));
				return true;
			}
			catch (nlohmann::json::parse_error& err) {
				log->Error("MaterialFileAsset::LoadMaterialFileJson - Could not parse file: \"", path, "\"! [Error: <", err.what(), ">]");
				return false;
			}
		}
	}

	class MaterialFileAsset::Importer : public virtual FileSystemDatabase::AssetImporter {
	private:
		GUID m_guid = GUID::Generate();
		std::mutex m_assetLock;
		Reference<MaterialFileAsset> m_asset;

		inline void InvalidateAsset(bool recreate) {
			std::unique_lock<std::mutex> assetLock(m_assetLock);
			if (m_asset != nullptr) {
				std::unique_lock<SpinLock> importerLock(m_asset->m_importerLock);
				m_asset->m_importer = nullptr;
			}
			if (recreate) {
				m_asset = new MaterialFileAsset(m_guid, this);
				m_asset->ReleaseRef();
			}
			else m_asset = nullptr;
		}

	public:
		inline virtual ~Importer() { InvalidateAsset(false); }

		inline virtual bool Import(Callback<const AssetInfo&> reportAsset) final override {
			if (m_asset == nullptr || m_asset->Guid() != m_guid)
				InvalidateAsset(true);
			const OS::Path path = AssetFilePath();
			{
				nlohmann::json json;
				if (!LoadMaterialFileJson(path, Log(), json)) return false;
			}
			{
				AssetInfo info;
				info.asset = m_asset;
				info.resourceName = OS::Path(AssetFilePath().stem());
				reportAsset(info);
				return true;
			}
		}

		inline static Reference<Importer> Get(MaterialFileAsset* asset) {
			std::unique_lock<SpinLock> importerLock(asset->m_importerLock);
			Reference<Importer> importer = asset->m_importer;
			return importer;
		}

		class Serializer;
	};

	class MaterialFileAsset::Importer::Serializer : public virtual FileSystemDatabase::AssetImporter::Serializer {
	public:
		inline Serializer()
			: Serialization::ItemSerializer(
				"MaterialFileAsset::Loader::Serializer[FileSystemDB]",
				"File System Database Material Asset Loader serializer") {}

		inline virtual Reference<FileSystemDatabase::AssetImporter> CreateReader() final override {
			return Object::Instantiate<Importer>();
		}

		inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, FileSystemDatabase::AssetImporter* target)const final override {
			if (target == nullptr) return;
			Importer* importer = dynamic_cast<Importer*>(target);
			if (importer == nullptr) {
				target->Log()->Error("MaterialFileAsset::Loader::Serializer::GetFields - Target not of the correct type!");
				return;
			}
			{
				static const Reference<const GUID::Serializer> serializer = Object::Instantiate<GUID::Serializer>("GUID", "GUID of the material");
				recordElement(serializer->Serialize(importer->m_guid));
			}
		}

		inline static Serializer* Instance() {
			static Serializer instance;
			return &instance;
		}
	};

	void TypeIdDetails::TypeDetails<MaterialFileAsset>::OnRegisterType() {
		MaterialFileAsset::Importer::Serializer::Instance()->Register(MaterialFileAsset::Extension());
	}

	void TypeIdDetails::TypeDetails<MaterialFileAsset>::OnUnregisterType() {
		MaterialFileAsset::Importer::Serializer::Instance()->Register(MaterialFileAsset::Extension());
	}

	template<> void TypeIdDetails::OnRegisterType<MaterialFileAsset>() {
		TypeIdDetails::TypeDetails<MaterialFileAsset>::OnRegisterType();
	}
	template<> void TypeIdDetails::OnUnregisterType<MaterialFileAsset>() {
		TypeIdDetails::TypeDetails<MaterialFileAsset>::OnUnregisterType();
	}

	namespace {
		static const Reference<const GUID::Serializer> GUID_SERIALIZER = Object::Instantiate<GUID::Serializer>(
			"MaterialFileAsset_ReferencedResourceId", "Resource ID, referenced by Material");
	}

	Reference<Material> MaterialFileAsset::LoadItem() {
		const Reference<Importer> importer = Importer::Get(this);
		if (importer == nullptr) return nullptr;

		const OS::Path path = importer->AssetFilePath();
		nlohmann::json json;
		if (!LoadMaterialFileJson(path, importer->Log(), json)) return nullptr;

		Reference<Material> material = Object::Instantiate<Material>(importer->GraphicsDevice());
		if (!Serialization::DeserializeFromJson(Material::Serializer::Instance()->Serialize(material), json, importer->Log(),
			[&](const Serialization::SerializedObject& object, const nlohmann::json& objectJson) -> bool {
				const Serialization::ObjectReferenceSerializer* serializer = object.As<Serialization::ObjectReferenceSerializer>();
				if (serializer == nullptr) {
					importer->Log()->Error("MaterialFileAsset::LoadItem - Unexpected serializer type!");
					return false;
				}
				const GUID initialGUID = [&]() {
					const Reference<Resource> referencedResource = dynamic_cast<Resource*>(serializer->GetObjectValue(object.TargetAddr()));
					return (referencedResource != nullptr && referencedResource->HasAsset()) ? referencedResource->GetAsset()->Guid() : GUID{};
				}();
				GUID guid = initialGUID;
				if (!Serialization::DeserializeFromJson(GUID_SERIALIZER->Serialize(guid), objectJson, importer->Log(),
					[&](const Serialization::SerializedObject&, const nlohmann::json&) -> bool {
						importer->Log()->Error("MaterialFileAsset::LoadItem - GUID Serializer not expected to reference Object pointers!");
						return false;
					})) return false;
				if (initialGUID != guid) {
					const Reference<Asset> referencedAsset = importer->FindAsset(guid);
					const Reference<Resource> referencedResource = (referencedAsset == nullptr) ? nullptr : referencedAsset->LoadResource();
					serializer->SetObjectValue(referencedResource, object.TargetAddr());
				}
				return true;
			})) {
			importer->Log()->Error("MaterialFileAsset::LoadItem - Failed to deserialize material!");
			return nullptr;
		}
		return material;
	}

	void MaterialFileAsset::Store(Material* resource) {
		const Reference<Importer> importer = Importer::Get(this);
		if (importer == nullptr) return;

		bool error = false;
		nlohmann::json json = Serialization::SerializeToJson(Material::Serializer::Instance()->Serialize(resource), importer->Log(), error,
			[&](const Serialization::SerializedObject& object, bool& err) -> nlohmann::json {
				const Serialization::ObjectReferenceSerializer* serializer = object.As<Serialization::ObjectReferenceSerializer>();
				if (serializer == nullptr) {
					importer->Log()->Error("MaterialFileAsset::Store - Unexpected serializer type!");
					err = true;
					return {};
				}
				GUID guid = [&]() {
					const Reference<Resource> referencedResource = dynamic_cast<Resource*>(serializer->GetObjectValue(object.TargetAddr()));
					return (referencedResource != nullptr && referencedResource->HasAsset()) ? referencedResource->GetAsset()->Guid() : GUID{};
				}();
				return Serialization::SerializeToJson(GUID_SERIALIZER->Serialize(guid), importer->Log(), err,
					[&](const Serialization::SerializedObject&, bool& e) -> nlohmann::json {
						importer->Log()->Error("MaterialFileAsset::Store - GUID Serializer not expected to reference Object pointers!");
						e = true;
						return {};
					});
			});
		if (error) {
			importer->Log()->Error("MaterialFileAsset::Store - Serialization error!");
			return;
		}

		const OS::Path assetPath = importer->AssetFilePath();
		std::ofstream fileStream(assetPath);
		if ((!fileStream.is_open()) || (fileStream.bad())) {
			importer->Log()->Error("MaterialFileAsset::Store - Could not open \"", assetPath, "\" for writing!");
			return;
		}
		else fileStream << json.dump(1, '\t') << std::endl;
		fileStream.close();
	}

	const OS::Path& MaterialFileAsset::Extension() {
		static const OS::Path extension = ".jimat";
		return extension;
	}

	MaterialFileAsset::MaterialFileAsset(const GUID& guid, Importer* importer) 
		: Asset(guid), m_importer(importer) {}
}

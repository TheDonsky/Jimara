#include "PhysicsMaterialFileAsset.h"
#include "../AssetDatabase/FileSystemDatabase/FileSystemDatabase.h"
#include <fstream>


namespace Jimara {
	class PhysicsMaterialFileAsset::Importer : public virtual FileSystemDatabase::AssetImporter {
	private:
		GUID m_guid = GUID::Generate();
		std::mutex m_assetLock;
		Reference<PhysicsMaterialFileAsset> m_asset;

		inline void InvalidateAsset(bool recreate) {
			std::unique_lock<std::mutex> assetLock(m_assetLock);
			if (m_asset != nullptr) {
				std::unique_lock<SpinLock> importerLock(m_asset->m_importerLock);
				m_asset->m_importer = nullptr;
			}
			if (recreate) {
				m_asset = new PhysicsMaterialFileAsset(m_guid, this);
				m_asset->ReleaseRef();
			}
			else m_asset = nullptr;
		}

	public:
		inline static bool LoadMaterialFileJson(const OS::Path& path, OS::Logger* log, nlohmann::json& json) {
			const Reference<OS::MMappedFile> memoryMapping = OS::MMappedFile::Create(path, log);
			if (memoryMapping == nullptr) {
				log->Error("PhysicsMaterialFileAsset::LoadMaterialFileJson - Failed to map file: \"", path, "\"!");
				return false;
			}
			try {
				MemoryBlock block(*memoryMapping);
				if (block.Size() > 0)
					json = nlohmann::json::parse(std::string_view(reinterpret_cast<const char*>(block.Data()), block.Size()));
				return true;
			}
			catch (nlohmann::json::parse_error& err) {
				log->Error("PhysicsMaterialFileAsset::LoadMaterialFileJson - Could not parse file: \"", path, "\"! [Error: <", err.what(), ">]");
				return false;
			}
		}

		inline virtual ~Importer() { 
			InvalidateAsset(false);
		}

		inline virtual bool Import(Callback<const AssetInfo&> reportAsset) final override {
			if (m_asset == nullptr || m_asset->Guid() != m_guid)
				InvalidateAsset(true);
			const OS::Path path = AssetFilePath();
			{
				nlohmann::json json;
				if (!LoadMaterialFileJson(path, Log(), json)) 
					return false;
				else {
					Reference<Physics::PhysicsMaterial> material = m_asset->GetLoaded();
					if (material != nullptr && (!DeserializeFromJson(material, this, Log(), json))) return false;
				}
			}
			{
				AssetInfo info;
				info.asset = m_asset;
				info.resourceName = OS::Path(AssetFilePath().stem());
				reportAsset(info);
				return true;
			}
		}

		inline static Reference<Importer> Get(PhysicsMaterialFileAsset* asset) {
			std::unique_lock<SpinLock> importerLock(asset->m_importerLock);
			Reference<Importer> importer = asset->m_importer;
			return importer;
		}

		inline static const Physics::PhysicsMaterial::Serializer& MaterialSerializer() {
			static const Physics::PhysicsMaterial::Serializer serializer("Physics Material", "Physics Material");
			return serializer;
		}

		class Serializer;
	};

	class PhysicsMaterialFileAsset::Importer::Serializer : public virtual FileSystemDatabase::AssetImporter::Serializer {
	public:
		inline Serializer()
			: Serialization::ItemSerializer(
				"PhysicsMaterialFileAsset::Loader::Serializer[FileSystemDB]",
				"File System Database Material Asset Loader serializer") {}

		inline virtual Reference<FileSystemDatabase::AssetImporter> CreateReader() final override {
			return Object::Instantiate<Importer>();
		}

		inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, FileSystemDatabase::AssetImporter* target)const final override {
			if (target == nullptr) return;
			Importer* importer = dynamic_cast<Importer*>(target);
			if (importer == nullptr) {
				target->Log()->Error("PhysicsMaterialFileAsset::Loader::Serializer::GetFields - Target not of the correct type!");
				return;
			}
			{
				static const Reference<const GUID::Serializer> serializer = Object::Instantiate<GUID::Serializer>("GUID", "GUID of the physics material");
				recordElement(serializer->Serialize(importer->m_guid));
			}
		}

		inline static Serializer* Instance() {
			static Serializer instance;
			return &instance;
		}
	};

	void TypeIdDetails::TypeDetails<PhysicsMaterialFileAsset>::OnRegisterType() {
		PhysicsMaterialFileAsset::Importer::Serializer::Instance()->Register(PhysicsMaterialFileAsset::Extension());
	}

	void TypeIdDetails::TypeDetails<PhysicsMaterialFileAsset>::OnUnregisterType() {
		PhysicsMaterialFileAsset::Importer::Serializer::Instance()->Register(PhysicsMaterialFileAsset::Extension());
	}

	template<> void TypeIdDetails::OnRegisterType<PhysicsMaterialFileAsset>() {
		TypeIdDetails::TypeDetails<PhysicsMaterialFileAsset>::OnRegisterType();
	}
	template<> void TypeIdDetails::OnUnregisterType<PhysicsMaterialFileAsset>() {
		TypeIdDetails::TypeDetails<PhysicsMaterialFileAsset>::OnUnregisterType();
	}

	namespace {
		static const Reference<const GUID::Serializer> GUID_SERIALIZER = Object::Instantiate<GUID::Serializer>(
			"PhysicsMaterialFileAsset_ReferencedResourceId", "Resource ID, referenced by Physics Material");
	}

	Reference<Physics::PhysicsMaterial> PhysicsMaterialFileAsset::LoadItem() {
		const Reference<Importer> importer = Importer::Get(this);
		if (importer == nullptr) 
			return nullptr;

		const OS::Path path = importer->AssetFilePath();
		nlohmann::json json;
		if (!Importer::LoadMaterialFileJson(path, importer->Log(), json)) 
			return nullptr;

		Reference<Physics::PhysicsMaterial> material = importer->PhysicsInstance()->CreateMaterial();
		if (!DeserializeFromJson(material, importer, importer->Log(), json)) {
			importer->Log()->Error("PhysicsMaterialFileAsset::LoadItem - Failed to deserialize physics material!");
			return nullptr;
		}

		return material;
	}

	void PhysicsMaterialFileAsset::Store(Physics::PhysicsMaterial* resource) {
		const Reference<Importer> importer = Importer::Get(this);
		if (importer == nullptr) return;

		bool error = false;
		nlohmann::json json = SerializeToJson(resource, importer->Log(), error);
		if (error) {
			importer->Log()->Error("PhysicsMaterialFileAsset::Store - Serialization error!");
			return;
		}

		const OS::Path assetPath = importer->AssetFilePath();
		std::ofstream fileStream((const std::filesystem::path&)assetPath);
		if ((!fileStream.is_open()) || (fileStream.bad())) {
			importer->Log()->Error("PhysicsMaterialFileAsset::Store - Could not open \"", assetPath, "\" for writing!");
			return;
		}
		else fileStream << json.dump(1, '\t') << std::endl;
		fileStream.close();
	}

	nlohmann::json PhysicsMaterialFileAsset::SerializeToJson(Physics::PhysicsMaterial* material, OS::Logger* log, bool& error) {
		return Serialization::SerializeToJson(Importer::MaterialSerializer().Serialize(material), log, error,
			[&](const Serialization::SerializedObject& object, bool& err) -> nlohmann::json {
				if (log != nullptr)
					log->Error("PhysicsMaterialFileAsset::SerializeToJson - Physics materials not expected to have object references!");
				err = true;
				return {};
			});
	}

	bool PhysicsMaterialFileAsset::DeserializeFromJson(Physics::PhysicsMaterial* material, AssetDatabase* database, OS::Logger* log, const nlohmann::json& serializedData) {
		return Serialization::DeserializeFromJson(Importer::MaterialSerializer().Serialize(material), serializedData, log,
			[&](const Serialization::SerializedObject& object, const nlohmann::json& objectJson) -> bool {
				if (log != nullptr)
					log->Error("PhysicsMaterialFileAsset::DeserializeFromJson - Physics materials not expected to have object references!");
				return false;
			});
	}

	const OS::Path& PhysicsMaterialFileAsset::Extension() {
		static const OS::Path extension = ".jiphysmat";
		return extension;
	}

	PhysicsMaterialFileAsset::PhysicsMaterialFileAsset(const GUID& guid, Importer* importer)
		: Asset(guid), m_importer(importer) {}
}

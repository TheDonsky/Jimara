#include "SceneAsset.h"
#include "../AssetDatabase/FileSystemDatabase/FileSystemDatabase.h"
#include "../Serialization/Helpers/ComponentHeirarchySerializer.h"
#include "../Serialization/Helpers/SerializeToJson.h"
#include "../../OS/IO/MMappedFile.h"
#include <fstream>


namespace Jimara {
	class SceneAsset::Importer : public virtual FileSystemDatabase::AssetImporter {
	private:
		GUID m_guid = GUID::Generate();
		std::mutex m_assetLock;
		Reference<SceneAsset> m_asset;

		inline void InvalidateAsset(bool recreate) {
			std::unique_lock<std::mutex> assetLock(m_assetLock);
			{
				std::unique_lock<SpinLock> importerLock(m_asset->m_importerLock);
				m_asset->m_importer = nullptr;
			}
			if (recreate) {
				m_asset = new SceneAsset(m_guid, this);
				m_asset->ReleaseRef();
			}
			else m_asset = nullptr;
		}

	public:
		inline virtual ~Importer() {
			InvalidateAsset(false);
		}

		inline virtual bool Import(Callback<const AssetInfo&> reportAsset) final override {
			if (m_asset != nullptr && m_asset->Guid() != m_guid)
				InvalidateAsset(true);
			Reference<Resource> resource = m_asset->Load();
			if (resource == nullptr) return false;
			else {
				AssetInfo info;
				info.asset = m_asset;
				info.resourceName = OS::Path(AssetFilePath().stem());
				reportAsset(info);
				return true;
			}
		}

		inline static Reference<const Importer> Get(SceneAsset* asset) {
			std::unique_lock<SpinLock> importerLock(asset->m_importerLock);
			Reference<const Importer> importer = asset->m_importer;
			return importer;
		}

		class Serializer;
	};

	class SceneAsset::Importer::Serializer : public virtual FileSystemDatabase::AssetImporter::Serializer {
	public:
		inline Serializer() 
			: Serialization::ItemSerializer(
				"SceneAsset::Loader::Serializer[FileSystemDB]",
				"File System Database Scene Asset Loader serializer") {}

		inline virtual Reference<FileSystemDatabase::AssetImporter> CreateReader() final override {
			return Object::Instantiate<Importer>();
		}

		inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, FileSystemDatabase::AssetImporter* target)const final override {
			if (target == nullptr) return;
			Importer* importer = dynamic_cast<Importer*>(target);
			if (importer == nullptr) {
				target->Log()->Error("SceneAsset::Loader::Serializer::GetFields - Target not of the correct type!");
				return;
			}
			{
				static const Reference<const GUID::Serializer> serializer = Object::Instantiate<GUID::Serializer>("GUID", "GUID of the [sub]scene file");
				serializer->Serialize(importer->m_guid);
			}
		}

		inline static Serializer* Instance() {
			static Serializer instance;
			return &instance;
		}

		inline static const OS::Path& Extension() {
			static const OS::Path extension = ".jimara";
			return extension;
		}
	};

	void TypeIdDetails::TypeDetails<SceneAsset>::OnRegisterType() {
		SceneAsset::Importer::Serializer::Instance()->Register(SceneAsset::Importer::Serializer::Extension());
	}

	void TypeIdDetails::TypeDetails<SceneAsset>::OnUnregisterType() {
		SceneAsset::Importer::Serializer::Instance()->Register(SceneAsset::Importer::Serializer::Extension());
	}

	template<> void TypeIdDetails::OnRegisterType<SceneAsset>() {
		TypeIdDetails::TypeDetails<SceneAsset>::OnRegisterType();
	}
	template<> void TypeIdDetails::OnUnregisterType<SceneAsset>() {
		TypeIdDetails::TypeDetails<SceneAsset>::OnUnregisterType();
	}

	namespace {
		class SceneAssetResource : public virtual ComponentHeirarchySpowner {
		public:
			const std::string name;
			std::mutex jsonLock;
			nlohmann::json json;

			inline SceneAssetResource(const std::string_view& nm) : name(nm) {}

			inline virtual Reference<Component> SpownHeirarchy(Component* parent, Callback<ProgressInfo> reportProgress, bool spownAsynchronous) final override {
				if (parent == nullptr) return nullptr;

				// TODO: Implement this crap!
				parent->Context()->Log()->Error("SceneAsset::SceneAssetResource::SpownHeirarchy - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				Unused(reportProgress, spownAsynchronous);
				return nullptr;
			}
		};
	}

	Reference<ComponentHeirarchySpowner> SceneAsset::LoadItem() {
		const Reference<const Importer> importer = Importer::Get(this);
		if (importer == nullptr) return nullptr;

		const OS::Path path = importer->AssetFilePath();
		const Reference<OS::MMappedFile> memoryMapping = OS::MMappedFile::Create(path, importer->Log());
		if (memoryMapping == nullptr) {
			importer->Log()->Error("SceneAsset::LoadItem - Failed to map file: \"", path, "\"!");
			return nullptr;
		}

		nlohmann::json json;
		try {
			MemoryBlock block(*memoryMapping);
			json = nlohmann::json::parse(std::string_view(reinterpret_cast<const char*>(block.Data()), block.Size()));
		}
		catch (nlohmann::json::parse_error& err) {
			importer->Log()->Error("SceneAsset::LoadItem - Could not parse file: \"", path, "\"! [Error: <", err.what(), ">]");
			return nullptr;
		}

		const std::string name = OS::Path(path.stem());
		Reference<SceneAssetResource> resource = Object::Instantiate<SceneAssetResource>(name);
		resource->json = std::move(json);
		return resource;
	}

	void SceneAsset::Store(ComponentHeirarchySpowner* resource) {
		const Reference<const Importer> importer = Importer::Get(this);
		if (importer == nullptr) return;

		SceneAssetResource* sceneResource = dynamic_cast<SceneAssetResource*>(resource);
		if (sceneResource == nullptr) {
			importer->Log()->Error("SceneAsset::Store - Unexpected resource type!");
			return;
		}

		const OS::Path assetPath = importer->AssetFilePath();
		std::ofstream fileStream(assetPath);
		if ((!fileStream.is_open()) || (fileStream.bad())) {
			importer->Log()->Error("SceneAsset::Store - Could not open \"", assetPath, "\" for writing!");
			return;
		}
		std::unique_lock<std::mutex> lock(sceneResource->jsonLock);
		fileStream << sceneResource->json.dump(1, '\t') << std::endl;
		fileStream.close();
	}

	SceneAsset::SceneAsset(const GUID& guid, const Importer* importer) 
		: Asset(guid), m_importer(importer) {}
}

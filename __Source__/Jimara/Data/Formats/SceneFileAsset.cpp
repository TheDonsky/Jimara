#include "SceneFileAsset.h"
#include "../AssetDatabase/FileSystemDatabase/FileSystemDatabase.h"
#include "../Serialization/Helpers/ComponentHeirarchySerializer.h"
#include "../Serialization/Helpers/SerializeToJson.h"
#include "../../OS/IO/MMappedFile.h"
#include <fstream>


namespace Jimara {
	class SceneFileAsset::Importer : public virtual FileSystemDatabase::AssetImporter {
	private:
		GUID m_guid = GUID::Generate();
		std::mutex m_assetLock;
		Reference<SceneFileAsset> m_asset;

		inline void InvalidateAsset(bool recreate) {
			std::unique_lock<std::mutex> assetLock(m_assetLock);
			if (m_asset != nullptr) {
				std::unique_lock<SpinLock> importerLock(m_asset->m_importerLock);
				m_asset->m_importer = nullptr;
			}
			if (recreate) {
				m_asset = new SceneFileAsset(m_guid, this);
				m_asset->ReleaseRef();
			}
			else m_asset = nullptr;
		}

	public:
		inline virtual ~Importer() {
			InvalidateAsset(false);
		}

		inline virtual bool Import(Callback<const AssetInfo&> reportAsset) final override {
			if (m_asset == nullptr || m_asset->Guid() != m_guid)
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

		inline static Reference<Importer> Get(SceneFileAsset* asset) {
			std::unique_lock<SpinLock> importerLock(asset->m_importerLock);
			Reference<Importer> importer = asset->m_importer;
			return importer;
		}

		class Serializer;
	};

	class SceneFileAsset::Importer::Serializer : public virtual FileSystemDatabase::AssetImporter::Serializer {
	public:
		inline Serializer() 
			: Serialization::ItemSerializer(
				"SceneFileAsset::Loader::Serializer[FileSystemDB]",
				"File System Database Scene Asset Loader serializer") {}

		inline virtual Reference<FileSystemDatabase::AssetImporter> CreateReader() final override {
			return Object::Instantiate<Importer>();
		}

		inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, FileSystemDatabase::AssetImporter* target)const final override {
			if (target == nullptr) return;
			Importer* importer = dynamic_cast<Importer*>(target);
			if (importer == nullptr) {
				target->Log()->Error("SceneFileAsset::Loader::Serializer::GetFields - Target not of the correct type!");
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

	void TypeIdDetails::TypeDetails<SceneFileAsset>::OnRegisterType() {
		SceneFileAsset::Importer::Serializer::Instance()->Register(SceneFileAsset::Importer::Serializer::Extension());
	}

	void TypeIdDetails::TypeDetails<SceneFileAsset>::OnUnregisterType() {
		SceneFileAsset::Importer::Serializer::Instance()->Register(SceneFileAsset::Importer::Serializer::Extension());
	}

	template<> void TypeIdDetails::OnRegisterType<SceneFileAsset>() {
		TypeIdDetails::TypeDetails<SceneFileAsset>::OnRegisterType();
	}
	template<> void TypeIdDetails::OnUnregisterType<SceneFileAsset>() {
		TypeIdDetails::TypeDetails<SceneFileAsset>::OnUnregisterType();
	}

	namespace {
		class SceneFileAssetResource : public virtual EditableComponentHeirarchySpowner {
		public:
			const std::string name;
			std::mutex dataLock;
			std::vector<Reference<Resource>> preloadedResources;
			nlohmann::json sceneJson;

			inline SceneFileAssetResource(const std::string_view& nm) : name(nm) {}

			inline ~SceneFileAssetResource() {}

			inline virtual Reference<Component> SpownHeirarchy(Component* parent) final override {
				if (parent == nullptr) return nullptr;

				ComponentHeirarchySerializerInput input;
				
				input.rootComponent = nullptr;
				input.context = parent->Context();
				
				std::pair<ComponentHeirarchySerializerInput*, Component*> onResourcesLoadedData(&input, parent);
				typedef void(*OnResourcesLoadedCallback)(decltype(onResourcesLoadedData)*);
				input.onResourcesLoaded = Callback(
					(OnResourcesLoadedCallback)[](decltype(onResourcesLoadedData)* data) {
						data->first->rootComponent = Object::Instantiate<Component>(data->second);
					}, &onResourcesLoadedData);
				
				std::pair<ComponentHeirarchySerializerInput*, const std::string*> onSerializationFinishedData(&input, &name);
				typedef void(*OnSerializationFinishedCallback)(decltype(onSerializationFinishedData)*);
				input.onSerializationFinished = Callback(
					(OnSerializationFinishedCallback)[](decltype(onSerializationFinishedData)* data) {
						if (data->first->rootComponent != nullptr)
							data->first->rootComponent->Name() = *data->second;
					}, &onSerializationFinishedData);

				const nlohmann::json snapshot = [&]() {
					std::unique_lock<std::mutex> snapshotLock(dataLock);
					nlohmann::json rv = sceneJson;
					return rv;
				}();

				if (!Serialization::DeserializeFromJson(ComponentHeirarchySerializer::Instance()->Serialize(input), snapshot, parent->Context()->Log(),
					[&](const Serialization::SerializedObject&, const nlohmann::json&) -> bool {
						parent->Context()->Log()->Error(
							"SceneFileAsset::SceneFileAssetResource::SpownHeirarchy - ComponentHeirarchySerializer is not expected to have object references!");
						return false;
					}))
					parent->Context()->Log()->Error("SceneFileAsset::SceneFileAssetResource::SpownHeirarchy - Failed to deserialize heirarchy! (Spowned data may be incomplete)");
				else if (input.rootComponent == nullptr)
					parent->Context()->Log()->Error("SceneFileAsset::SceneFileAssetResource::SpownHeirarchy - Failed to create heirarchy!");

				{
					std::unique_lock<std::mutex> snapshotLock(dataLock);
					preloadedResources = std::move(input.resources);
				}

				return input.rootComponent;
			}

			virtual void StoreHeirarchyData(Component* parent) final override {
				nlohmann::json snapshot;
				ComponentHeirarchySerializerInput input;

				if (parent == nullptr)
					snapshot = {};
				else {
					input.rootComponent = parent;
					bool error = false;
					snapshot = Serialization::SerializeToJson(
						ComponentHeirarchySerializer::Instance()->Serialize(input), parent->Context()->Log(), error,
						[&](const Serialization::SerializedObject&, bool& error) -> nlohmann::json {
							parent->Context()->Log()->Error(
								"SceneFileAsset::SceneFileAssetResource::StoreHeirarchyData - ComponentHeirarchySerializer is not expected to have any Component references!");
							error = true;
							return {};
						});
					if (error) {
						parent->Context()->Log()->Error("SceneFileAsset::SceneFileAssetResource::StoreHeirarchyData - Failed to create scene snapshot!");
						return;
					}
				}

				{
					std::unique_lock<std::mutex> snapshotLock(dataLock);
					preloadedResources = std::move(input.resources);
					sceneJson = std::move(snapshot);
				}
			}
		};

		template<typename Lambda>
		inline static Callback<Asset::LoadInfo> ReportProgressCallback(const Lambda& lambda) {
			typedef void(*CallLambda)(const Lambda*, Asset::LoadInfo);
			static const CallLambda callLambda = [](const Lambda* lambda, Asset::LoadInfo info) {
				(*lambda)(info);
			};
			return Callback<Asset::LoadInfo>(callLambda, &lambda);
		}
	}

	Reference<EditableComponentHeirarchySpowner> SceneFileAsset::LoadItem() {
		const Reference<Importer> importer = Importer::Get(this);
		if (importer == nullptr) return nullptr;

		const OS::Path path = importer->AssetFilePath();
		const Reference<OS::MMappedFile> memoryMapping = OS::MMappedFile::Create(path, importer->Log());
		if (memoryMapping == nullptr) {
			importer->Log()->Error("SceneFileAsset::LoadItem - Failed to map file: \"", path, "\"!");
			return nullptr;
		}

		nlohmann::json json;
		try {
			MemoryBlock block(*memoryMapping);
			json = nlohmann::json::parse(std::string_view(reinterpret_cast<const char*>(block.Data()), block.Size()));
		}
		catch (nlohmann::json::parse_error& err) {
			importer->Log()->Error("SceneFileAsset::LoadItem - Could not parse file: \"", path, "\"! [Error: <", err.what(), ">]");
			return nullptr;
		}

		// Preload resources:
		ComponentHeirarchySerializerInput input;
		{
			input.assetDatabase = m_importer;
			auto lambda = [&](const Asset::LoadInfo& info) { ReportProgress(info); };
			input.reportProgress = ReportProgressCallback(lambda);
			if (!Serialization::DeserializeFromJson(ComponentHeirarchySerializer::Instance()->Serialize(input), json, importer->Log(),
				[&](const Serialization::SerializedObject&, const nlohmann::json&) -> bool {
					importer->Log()->Error(
						"SceneFileAsset::LoadItem - ComponentHeirarchySerializer is not expected to have object references!");
					return false;
				}))
				importer->Log()->Error("SceneFileAsset::LoadItem - Failed to preload assets!");
		}

		const std::string name = OS::Path(path.stem());
		Reference<SceneFileAssetResource> resource = Object::Instantiate<SceneFileAssetResource>(name);
		resource->preloadedResources = std::move(input.resources);
		resource->sceneJson = std::move(json);
		return resource;
	}

	void SceneFileAsset::Store(EditableComponentHeirarchySpowner* resource) {
		const Reference<const Importer> importer = Importer::Get(this);
		if (importer == nullptr) return;

		SceneFileAssetResource* sceneResource = dynamic_cast<SceneFileAssetResource*>(resource);
		if (sceneResource == nullptr) {
			importer->Log()->Error("SceneFileAsset::Store - Unexpected resource type!");
			return;
		}

		const OS::Path assetPath = importer->AssetFilePath();
		std::ofstream fileStream(assetPath);
		if ((!fileStream.is_open()) || (fileStream.bad())) {
			importer->Log()->Error("SceneFileAsset::Store - Could not open \"", assetPath, "\" for writing!");
			return;
		}
		{
			std::unique_lock<std::mutex> lock(sceneResource->dataLock);
			fileStream << sceneResource->sceneJson.dump(1, '\t') << std::endl;
		}
		fileStream.close();
	}

	SceneFileAsset::SceneFileAsset(const GUID& guid, Importer* importer) 
		: Asset(guid), m_importer(importer) {}
}

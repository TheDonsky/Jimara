#include "ConfigurableResourceFileAsset.h"
#include "../AssetDatabase/FileSystemDatabase/FileSystemDatabase.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include <fstream>


namespace Jimara {
	class ConfigurableResourceFileAsset::Importer : public virtual FileSystemDatabase::AssetImporter {
	private:
		GUID m_guid = GUID::Generate();
		std::mutex m_assetLock;
		Reference<ConfigurableResourceFileAsset> m_asset;

		inline void InvalidateAsset(bool recreate, const std::string_view resourceType) {
			std::unique_lock<std::mutex> assetLock(m_assetLock);
			if (m_asset != nullptr) {
				std::unique_lock<SpinLock> importerLock(m_asset->m_importerLock);
				m_asset->m_importer = nullptr;
			}
			if (recreate) {
				m_asset = new ConfigurableResourceFileAsset(m_guid, this, resourceType);
				m_asset->ReleaseRef();
			}
			else m_asset = nullptr;
		}

	public:
		inline virtual ~Importer() { InvalidateAsset(false, ""); }

		virtual bool Import(Callback<const AssetInfo&> reportAsset) final override {
			const OS::Path filePath = AssetFilePath();
			Reference<const ConfigurableResource::ResourceFactory> factory = nullptr;
			{
				nlohmann::json data;
				if (LoadJsonFromFile(filePath, Log(), data)) {
					ConfigurableResource::SerializableInstance instance = SerializableInstance(nullptr);
					DeserializeFromJson(&instance, nullptr, Log(), data);
					if (instance.instance != nullptr) {
						const Reference<const ConfigurableResource::ResourceFactory::Set> factories = ConfigurableResource::ResourceFactory::All();
						factory = factories->FindFactory(instance.instance.operator->());
					}
				}
			}
			const TypeId resourceType = (factory == nullptr) ? TypeId::Of<ConfigurableResource>() : factory->InstanceType();

			if (m_asset == nullptr || m_asset->Guid() != m_guid || m_asset->m_typeName != resourceType.Name())
				InvalidateAsset(true, resourceType.Name());
			
			AssetInfo info;
			info.asset = m_asset;
			info.resourceName = OS::Path(filePath.stem());
			reportAsset(info);
			return true;
		}

		inline static Reference<Importer> GetImporter(ConfigurableResourceFileAsset* asset) {
			assert(asset != nullptr);
			std::unique_lock<SpinLock> lock(asset->m_importerLock);
			const Reference<Importer> importer(asset->m_importer);
			return importer;
		}

		inline static bool LoadJsonFromFile(const OS::Path& path, OS::Logger* log, nlohmann::json& json) {
			const Reference<OS::MMappedFile> memoryMapping = OS::MMappedFile::Create(path, log);
			if (memoryMapping == nullptr) {
				log->Error("ConfigurableResourceAsset::Importer::LoadJsonFromFile - Failed to map file: \"", path, "\"!");
				return false;
			}
			try {
				MemoryBlock block(*memoryMapping);
				if (block.Size() > 0)
					json = nlohmann::json::parse(std::string_view(reinterpret_cast<const char*>(block.Data()), block.Size()));
				return true;
			}
			catch (nlohmann::json::parse_error& err) {
				log->Error("ConfigurableResourceAsset::Importer::LoadJsonFromFile - Could not parse file: \"", path, "\"! [Error: <", err.what(), ">]");
				return false;
			}
		}

		static const GUID::Serializer* GuidSerializer() {
			static const GUID::Serializer serializer("ConfigurableResourceAsset_ReferencedResourceId", "Resource ID, referenced by ConfigurableResource");
			return &serializer;
		}

		inline ConfigurableResource::SerializableInstance SerializableInstance(ConfigurableResource* resource)const {
			ConfigurableResource::SerializableInstance inst = {};
			inst.instance = resource;
			inst.recreateArgs.log = Log();
			inst.recreateArgs.graphicsDevice = GraphicsDevice();
			inst.recreateArgs.shaderLibrary = ShaderLibrary();
			inst.recreateArgs.physicsInstance = PhysicsInstance();
			inst.recreateArgs.audioDevice = AudioDevice();
			return inst;
		}

		class Serializer;
	};


	class ConfigurableResourceFileAsset::Importer::Serializer : public virtual FileSystemDatabase::AssetImporter::Serializer {
	public:
		inline Serializer() : Serialization::ItemSerializer(
			"ConfigurableResourceAsset::Importer::Serializer[FileSystemDB]",
			"File System Database Configurable Resource Asset Loader serializer") {}

		inline virtual ~Serializer() {}

		inline virtual Reference<FileSystemDatabase::AssetImporter> CreateReader() final override {
			return Object::Instantiate<Importer>();
		}

		inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, FileSystemDatabase::AssetImporter* target)const final override {
			if (target == nullptr) return;
			Importer* importer = dynamic_cast<Importer*>(target);
			if (importer == nullptr) {
				target->Log()->Error("ConfigurableResourceAsset::Importer::Serializer::GetFields - Target not of the correct type!");
				return;
			}
			{
				static const Reference<const GUID::Serializer> serializer = Object::Instantiate<GUID::Serializer>("GUID", "GUID of the configurable resource");
				recordElement(serializer->Serialize(importer->m_guid));
			}
		}

		inline static Serializer* Instance() {
			static Serializer instance;
			return &instance;
		}
	};

	void TypeIdDetails::TypeDetails<ConfigurableResourceFileAsset>::OnRegisterType() {
		ConfigurableResourceFileAsset::Importer::Serializer::Instance()->Register(ConfigurableResourceFileAsset::Extension());
	}

	void TypeIdDetails::TypeDetails<ConfigurableResourceFileAsset>::OnUnregisterType() {
		ConfigurableResourceFileAsset::Importer::Serializer::Instance()->Register(ConfigurableResourceFileAsset::Extension());
	}

	template<> void TypeIdDetails::OnRegisterType<ConfigurableResourceFileAsset>() {
		TypeIdDetails::TypeDetails<ConfigurableResourceFileAsset>::OnRegisterType();
	}
	template<> void TypeIdDetails::OnUnregisterType<ConfigurableResourceFileAsset>() {
		TypeIdDetails::TypeDetails<ConfigurableResourceFileAsset>::OnUnregisterType();
	}

	TypeId ConfigurableResourceFileAsset::ResourceType()const {
		const Reference<ConfigurableResource::ResourceFactory::Set> factories = ConfigurableResource::ResourceFactory::All();
		const Reference<const ConfigurableResource::ResourceFactory> factory = factories->FindFactory(m_typeName);
		return factory == nullptr ? TypeId::Of<ConfigurableResource>() : factory->InstanceType();
	}

	Reference<Resource> ConfigurableResourceFileAsset::LoadResourceObject() {
		const Reference<Importer> importer = Importer::GetImporter(this);
		if (importer == nullptr) 
			return nullptr;

		const OS::Path path = importer->AssetFilePath();
		nlohmann::json json;
		if (!Importer::LoadJsonFromFile(path, importer->Log(), json)) 
			return Object::Instantiate<ConfigurableResource>();

		ConfigurableResource::SerializableInstance resource = importer->SerializableInstance(nullptr);
		if (!DeserializeFromJson(&resource, importer, importer->Log(), json)) {
			importer->Log()->Error("ConfigurableResourceAsset::LoadItem - Failed to deserialize data!");
			return nullptr;
		}

		if (resource.instance != nullptr) {
			const Reference<ConfigurableResource::ResourceFactory::Set> factories = ConfigurableResource::ResourceFactory::All();
			const Reference<const ConfigurableResource::ResourceFactory> factory = factories->FindFactory(resource.instance.operator->());
			if (factory == nullptr || factory->InstanceType().Name() != m_typeName) {
				//importer->Log()->Error("ConfigurableResourceAsset::LoadItem - Loaded instance of an incorrect type ",
				//	"(Expected: ", m_typeName, "; Got: ", factory == nullptr ? std::string_view("<None>") : factory->InstanceType().Name(), ")!");
				return nullptr;
			}
			return resource.instance;
		}
		else return Object::Instantiate<ConfigurableResource>();
	}

	void ConfigurableResourceFileAsset::RefreshExternalDependencies(Resource* res) {
		ConfigurableResource* const configurable = dynamic_cast<ConfigurableResource*>(res);
		const Reference<Importer> importer = Importer::GetImporter(this);
		if (importer == nullptr || configurable == nullptr)
			return;
		auto processField = [&](const Serialization::SerializedObject& object) {
			const Serialization::ObjectReferenceSerializer* serializer = object.As<Serialization::ObjectReferenceSerializer>();
			if (serializer == nullptr) 
				return;
			Reference<Object> item = serializer->GetObjectValue(object.TargetAddr());
			Reference<Resource> resource = item;
			Reference<Asset> asset;
			if (resource != nullptr) 
				asset = resource->GetAsset();
			if (asset == nullptr) 
				asset = item;
			if (asset == nullptr) 
				return;
			Reference<Asset> newAsset = importer->FindAsset(asset->Guid());
			if (newAsset == nullptr)
				item = nullptr;
			else if (resource != nullptr && asset == resource->GetAsset())
				item = newAsset->LoadResource();
			else item = newAsset;
			serializer->SetObjectValue(item, object.TargetAddr());
		};
		static const ConfigurableResource::Serializer serializer("ConfigurableResourceAsset::ReloadExternalDependencies");
		serializer.GetFields(Callback<Serialization::SerializedObject>::FromCall(&processField), configurable);
	}

	void ConfigurableResourceFileAsset::StoreResource() {
		Reference<ConfigurableResource> resource = GetLoadedAs<ConfigurableResource>();
		if (resource == nullptr)
			return;

		const Reference<Importer> importer = Importer::GetImporter(this);
		if (importer == nullptr) 
			return;

		bool error = false;
		ConfigurableResource::SerializableInstance instance = importer->SerializableInstance(resource);
		nlohmann::json json = SerializeToJson(&instance, importer->Log(), error);
		if (error) {
			importer->Log()->Error("ConfigurableResourceAsset::Store - Serialization error!");
			return;
		}

		const OS::Path assetPath = importer->AssetFilePath();
		std::ofstream fileStream(assetPath);
		if ((!fileStream.is_open()) || (fileStream.bad())) {
			importer->Log()->Error("ConfigurableResourceAsset::Store - Could not open \"", assetPath, "\" for writing!");
			return;
		}
		else fileStream << json.dump(1, '\t') << std::endl;
		fileStream.close();
	}

	nlohmann::json ConfigurableResourceFileAsset::SerializeToJson(ConfigurableResource::SerializableInstance* resource, OS::Logger* log, bool& error) {
		return Serialization::SerializeToJson(ConfigurableResource::InstanceSerializer::Instance()->Serialize(resource), log, error,
			[&](const Serialization::SerializedObject& object, bool& err) -> nlohmann::json {
				const Serialization::ObjectReferenceSerializer* serializer = object.As<Serialization::ObjectReferenceSerializer>();
				if (serializer == nullptr) {
					if (log != nullptr) 
						log->Error("ConfigurableResourceAsset::SerializeToJson - Unexpected serializer type!");
					err = true;
					return {};
				}
				GUID guid = [&]() {
					const Reference<Resource> referencedResource = dynamic_cast<Resource*>(serializer->GetObjectValue(object.TargetAddr()));
					return (referencedResource != nullptr && referencedResource->HasAsset()) ? referencedResource->GetAsset()->Guid() : GUID{};
					}();
					return Serialization::SerializeToJson(Importer::GuidSerializer()->Serialize(guid), log, err,
						[&](const Serialization::SerializedObject&, bool& e) -> nlohmann::json {
							if (log != nullptr) 
								log->Error("ConfigurableResourceAsset::SerializeToJson - GUID Serializer not expected to reference Object pointers!");
							e = true;
							return {};
						});
			});
	}

	bool ConfigurableResourceFileAsset::DeserializeFromJson(
		ConfigurableResource::SerializableInstance* resource, AssetDatabase* database, OS::Logger * log, const nlohmann::json & serializedData) {
		return Serialization::DeserializeFromJson(ConfigurableResource::InstanceSerializer::Instance()->Serialize(resource), serializedData, log,
			[&](const Serialization::SerializedObject& object, const nlohmann::json& objectJson) -> bool {
				const Serialization::ObjectReferenceSerializer* serializer = object.As<Serialization::ObjectReferenceSerializer>();
				if (serializer == nullptr) {
					if (log != nullptr) 
						log->Error("ConfigurableResourceAsset::DeserializeFromJson - Unexpected serializer type!");
					return false;
				}
				const GUID initialGUID = [&]() {
					const Reference<Resource> referencedResource = dynamic_cast<Resource*>(serializer->GetObjectValue(object.TargetAddr()));
					return (referencedResource != nullptr && referencedResource->HasAsset()) ? referencedResource->GetAsset()->Guid() : GUID{};
					}();
					GUID guid = initialGUID;
					if (!Serialization::DeserializeFromJson(Importer::GuidSerializer()->Serialize(guid), objectJson, log,
						[&](const Serialization::SerializedObject&, const nlohmann::json&) -> bool {
							if (log != nullptr) 
								log->Error("ConfigurableResourceAsset::DeserializeFromJson - GUID Serializer not expected to reference Object pointers!");
							return false;
						})) return false;
					if (initialGUID != guid) {
						const Reference<Asset> referencedAsset = (database != nullptr) ? database->FindAsset(guid) : Reference<Asset>(nullptr);
						const Reference<Resource> referencedResource = (referencedAsset == nullptr) ? nullptr : referencedAsset->LoadResource();
						serializer->SetObjectValue(referencedResource, object.TargetAddr());
					}
					return true;
			});
	}

	const OS::Path& ConfigurableResourceFileAsset::Extension() {
		static const OS::Path extension = ".jiconf";
		return extension;
	}

	ConfigurableResourceFileAsset::ConfigurableResourceFileAsset(const GUID& guid, Importer* importer, const std::string_view& typeName)
		: Asset(guid), m_typeName(typeName), m_importer(importer) {}
}

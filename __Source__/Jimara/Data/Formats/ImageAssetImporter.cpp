#include "ImageAssetImporter.h"
#include "../AssetDatabase/FileSystemDatabase/FileSystemDatabase.h"
#include "../Serialization/Attributes/EnumAttribute.h"
#include "../../Environment/Rendering/ImageBasedLighting/HDRIEnvironment.h"


namespace Jimara {
	namespace {
		class ImageAssetSerializer;
		class ImageAssetReader;

		class ImageAsset : public virtual Asset::Of<Graphics::TextureSampler> {
		private:
			const Reference<const ImageAssetReader> m_reader;

		public:
			inline ImageAsset(const ImageAssetReader* reader);
			inline virtual Reference<Graphics::TextureSampler> LoadItem() override;
		};

		class HDRIEnvironmentAsset : public virtual Asset::Of<HDRIEnvironment> {
		private:
			const Reference<const ImageAssetReader> m_reader;
			const Reference<ImageAsset> m_imageAsset;

		public:
			inline HDRIEnvironmentAsset(const ImageAssetReader* reader, ImageAsset* image);
			inline virtual Reference<HDRIEnvironment> LoadItem() override;
		};

		class ImageAssetReader : public virtual FileSystemDatabase::AssetImporter {
		private:
			GUID m_guid = GUID::Generate();
			bool m_createMipmaps = true;
			Graphics::TextureSampler::FilteringMode m_filtering = Graphics::TextureSampler::FilteringMode::LINEAR;
			GUID m_hdriEnvironmentGUID = GUID::Generate();

			friend class ImageAssetSerializer;
			friend class ImageAsset;
			friend class HDRIEnvironmentAsset;

		public:
			inline virtual bool Import(Callback<const AssetInfo&> reportAsset) final override {
				Reference<ImageAsset> asset = Object::Instantiate<ImageAsset>(this);
				Reference<HDRIEnvironmentAsset> hdriEnvironmentAsset = Object::Instantiate<HDRIEnvironmentAsset>(this, asset);
				
				static const std::string alreadyLoadedState = "Imported";
				if (PreviousImportData() != alreadyLoadedState) {
					Reference<Resource> resource = asset->Load();
					if (resource == nullptr)
						return false;
					else PreviousImportData() = alreadyLoadedState;
				}
				
				AssetInfo info;
				info.asset = asset;
				reportAsset(info);

				AssetInfo hdriInfo = {};
				hdriInfo.asset = hdriEnvironmentAsset;
				reportAsset(hdriInfo);

				return true;
			}
		};

		inline ImageAsset::ImageAsset(const ImageAssetReader* reader) : Asset(reader->m_guid), m_reader(reader) {}
		inline Reference<Graphics::TextureSampler> ImageAsset::LoadItem() {
			static const std::unordered_set<OS::Path> highPrecisionExtensions = {
				".hdr"
			};
			Reference<Graphics::ImageTexture> texture = Graphics::ImageTexture::LoadFromFile(
				m_reader->GraphicsDevice(), m_reader->AssetFilePath(), m_reader->m_createMipmaps,
				highPrecisionExtensions.find(m_reader->AssetFilePath().extension()) != highPrecisionExtensions.end());
			if (texture == nullptr) return nullptr;
			return texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D)->CreateSampler(m_reader->m_filtering);
		}

		inline HDRIEnvironmentAsset::HDRIEnvironmentAsset(const ImageAssetReader* reader, ImageAsset* image) 
			: Asset(reader->m_hdriEnvironmentGUID), m_reader(reader), m_imageAsset(image) {}
		inline Reference<HDRIEnvironment> HDRIEnvironmentAsset::LoadItem() {
			Reference<Graphics::TextureSampler> sampler = m_imageAsset->LoadItem();
			if (sampler == nullptr) return nullptr;
			else return HDRIEnvironment::Create(m_reader->GraphicsDevice(), m_reader->ShaderLoader(), sampler);
		}

		class ImageAssetSerializer : public virtual FileSystemDatabase::AssetImporter::Serializer {
		public:
			inline ImageAssetSerializer() : Serialization::ItemSerializer("ImageAssetSerializer") {}

			inline virtual Reference<FileSystemDatabase::AssetImporter> CreateReader() final override {
				return Object::Instantiate<ImageAssetReader>();
			}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, FileSystemDatabase::AssetImporter* target)const final override {
				if (target == nullptr) return;
				ImageAssetReader* importer = dynamic_cast<ImageAssetReader*>(target);
				if (importer == nullptr) {
					target->Log()->Error("OBJAssetImporterSerializer::GetFields - Target not of the correct type!");
					return;
				}
				{
					static const Reference<const GUID::Serializer> serializer = Object::Instantiate<GUID::Serializer>("ImageGUID");
					recordElement(serializer->Serialize(importer->m_guid));
				}
				{
					static const Reference<const Serialization::ItemSerializer::Of<bool>> serializer = Serialization::ValueSerializer<bool>::Create("CreateMipmaps");
					recordElement(serializer->Serialize(importer->m_createMipmaps));
				}
				{
					static const Reference<const Serialization::ItemSerializer::Of<uint8_t>> serializer = Serialization::ValueSerializer<uint8_t>::Create(
						"Filtering", {}, { Object::Instantiate<Serialization::EnumAttribute<uint8_t>>(std::vector<Serialization::EnumAttribute<uint8_t>::Choice> {
						Serialization::EnumAttribute<uint8_t>::Choice("NEAREST", static_cast<uint8_t>(Graphics::TextureSampler::FilteringMode::NEAREST)),
						Serialization::EnumAttribute<uint8_t>::Choice("LINEAR", static_cast<uint8_t>(Graphics::TextureSampler::FilteringMode::LINEAR))
					}, false) });
					uint8_t filtering = static_cast<uint8_t>(importer->m_filtering);
					recordElement(serializer->Serialize(filtering));
					importer->m_filtering = static_cast<Graphics::TextureSampler::FilteringMode>(
						filtering < static_cast<uint8_t>(Graphics::TextureSampler::FilteringMode::FILTER_COUNT) ?
						filtering : static_cast<uint8_t>(Graphics::TextureSampler::FilteringMode::LINEAR));
				}
				{
					static const Reference<const GUID::Serializer> serializer = Object::Instantiate<GUID::Serializer>("HDRIEnvironmentGUID");
					recordElement(serializer->Serialize(importer->m_hdriEnvironmentGUID));
				}
			}
			
			inline static ImageAssetSerializer* Instance() {
				static const Reference<ImageAssetSerializer> instance = Object::Instantiate<ImageAssetSerializer>();
				return instance;
			}

			inline static const std::vector<OS::Path>& SupportedFormats() {
				static const std::vector<OS::Path> formats = { 
					OS::Path(".jpg"),
					OS::Path(".png"),
					OS::Path(".tga"),
					OS::Path(".bmp"),
					OS::Path(".psd"),
					OS::Path(".gif"),
					OS::Path(".hdr")
				};
				return formats;
			}

			template<typename CallbackType>
			inline static void ForEachFormat(const CallbackType& call) {
				const std::vector<OS::Path>& formats = ImageAssetSerializer::SupportedFormats();
				for (size_t i = 0; i < formats.size(); i++)
					call(formats[i]);
			}
		};
	}

	template<> void TypeIdDetails::OnRegisterType<ImageAssetImporter>() {
		ImageAssetSerializer::ForEachFormat([](const OS::Path& extension) { ImageAssetSerializer::Instance()->Register(extension); });
	}
	template<> void TypeIdDetails::OnUnregisterType<ImageAssetImporter>() {
		ImageAssetSerializer::ForEachFormat([](const OS::Path& extension) { ImageAssetSerializer::Instance()->Unregister(extension); });

	}
}

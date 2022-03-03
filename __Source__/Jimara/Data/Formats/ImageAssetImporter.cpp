#include "ImageAssetImporter.h"
#include "../AssetDatabase/FileSystemDatabase/FileSystemDatabase.h"


namespace Jimara {
	namespace {
		class ImageAssetSerializer;
		class ImageAssetReader;

		class ImageAsset : public virtual Asset::Of<Graphics::ImageTexture> {
		private:
			const Reference<const ImageAssetReader> m_reader;

		public:
			inline ImageAsset(const ImageAssetReader* reader);
			inline virtual Reference<Graphics::ImageTexture> LoadItem() override;
		};

		class ImageAssetReader : public virtual FileSystemDatabase::AssetImporter {
		private:
			GUID m_guid = GUID::Generate();
			bool m_createMipmaps = true;

			friend class ImageAssetSerializer;
			friend class ImageAsset;

		public:
			inline virtual bool Import(Callback<const AssetInfo&> reportAsset) final override {
				Reference<ImageAsset> asset = Object::Instantiate<ImageAsset>(this);
				Reference<Resource> resource = asset->Load();
				if (resource == nullptr) return false;
				else {
					AssetInfo info;
					info.asset = asset;
					reportAsset(info);
					return true;
				}
			}
		};

		inline ImageAsset::ImageAsset(const ImageAssetReader* reader) : Asset(reader->m_guid), m_reader(reader) {}
		inline Reference<Graphics::ImageTexture> ImageAsset::LoadItem() {
			return Graphics::ImageTexture::LoadFromFile(m_reader->GraphicsDevice(), m_reader->AssetFilePath(), m_reader->m_createMipmaps);
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
					OS::Path(".gif")
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

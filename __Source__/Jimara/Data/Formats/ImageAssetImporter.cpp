#include "ImageAssetImporter.h"
#include "../AssetDatabase/FileSystemDatabase/FileSystemDatabase.h"
#include "../Serialization/Attributes/EnumAttribute.h"
#include "../Serialization/Helpers/SerializerMacros.h"
#include "../../Environment/Rendering/ImageBasedLighting/HDRIEnvironment.h"


namespace Jimara {
	namespace {
		class ImageAssetSerializer;
		class ImageAssetReader;

		class ImageAsset : public virtual Asset::Of<Graphics::TextureSampler> {
		private:
			const Reference<ImageAssetReader> m_reader;

		public:
			inline ImageAsset(ImageAssetReader* reader);
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

			static_assert(std::is_same_v<std::underlying_type_t<Graphics::ImageTexture::ImportMode>, uint8_t>);
			static_assert(!std::is_same_v<std::underlying_type_t<Graphics::ImageTexture::ImportMode>, int8_t>);
			Graphics::ImageTexture::ImportMode m_importMode = static_cast<Graphics::ImageTexture::ImportMode>(~uint8_t(0u));

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

			inline Graphics::ImageTexture::ImportMode GetImportMode() {
				const Graphics::ImageTexture::ImportMode maxImportMode = Math::Max(
					Graphics::ImageTexture::ImportMode::SDR_SRGB,
					Graphics::ImageTexture::ImportMode::SDR_LINEAR,
					Graphics::ImageTexture::ImportMode::HDR);
				if (m_importMode <= maxImportMode)
					return m_importMode;

				static const std::unordered_set<OS::Path> hdrExtensions = {
					".hdr"
				};
				static const std::unordered_set<std::string_view> linearHints = {
					"normal",
					"normals",
					"height",
					"roughness",
					"smoothness",
					"ao"
				};
				static const std::string breakSymbols = "/\\-_ \t.";

				const OS::Path path = AssetFilePath();
				if (hdrExtensions.find(path.extension()) != hdrExtensions.end())
					return Graphics::ImageTexture::ImportMode::HDR;
				else {
					const std::string pathStr = OS::Path(path.filename());
					size_t wordStart = 0u;
					while (wordStart < pathStr.length()) {
						if (breakSymbols.find(pathStr[wordStart]) != std::string::npos) {
							wordStart++;
							continue;
						}
						size_t wordEnd = wordStart + 1u;
						while (wordEnd < pathStr.length() &&
							breakSymbols.find(pathStr[wordEnd]) == std::string::npos)
							wordEnd++;
						const std::string_view token = std::string_view(pathStr.c_str() + wordStart, wordEnd - wordStart);
						if (linearHints.find(token) != linearHints.end())
							return Graphics::ImageTexture::ImportMode::SDR_LINEAR;
						wordStart = wordEnd;
					}
				}
				return Graphics::ImageTexture::ImportMode::SDR_SRGB;
			}
		};

		inline ImageAsset::ImageAsset(ImageAssetReader* reader) : Asset(reader->m_guid), m_reader(reader) {}
		inline Reference<Graphics::TextureSampler> ImageAsset::LoadItem() {
			static const std::unordered_set<OS::Path> highPrecisionExtensions = {
				".hdr"
			};
			Reference<Graphics::ImageTexture> texture = Graphics::ImageTexture::LoadFromFile(
				m_reader->GraphicsDevice(), m_reader->AssetFilePath(), m_reader->m_createMipmaps, m_reader->GetImportMode());
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
				JIMARA_SERIALIZE_FIELDS(importer, recordElement) {
					JIMARA_SERIALIZE_FIELD(importer->m_guid, "ImageGUID", "Image Identifier");
					JIMARA_SERIALIZE_FIELD(importer->m_hdriEnvironmentGUID, "HDRIEnvironmentGUID", "HDRI Environment Identifier");
					JIMARA_SERIALIZE_FIELD(importer->m_createMipmaps, "CreateMipmaps", "If true, Mip chain will be created");
					JIMARA_SERIALIZE_FIELD(importer->m_filtering, "Filtering", "Sampling mode",
						Object::Instantiate<Serialization::EnumAttribute<std::underlying_type_t<decltype(importer->m_filtering)>>>(false,
							"NEAREST", Graphics::TextureSampler::FilteringMode::NEAREST,
							"LINEAR", Graphics::TextureSampler::FilteringMode::LINEAR));
					JIMARA_SERIALIZE_FIELD(importer->m_importMode, "Import mode", "Import mode information",
						Object::Instantiate<Serialization::EnumAttribute<std::underlying_type_t<decltype(importer->m_importMode)>>>(false,
							"SDR_SRGB", Graphics::ImageTexture::ImportMode::SDR_SRGB,
							"SDR_LINEAR", Graphics::ImageTexture::ImportMode::SDR_LINEAR,
							"HDR", Graphics::ImageTexture::ImportMode::HDR,
							"AUTO", static_cast<Graphics::ImageTexture::ImportMode>(~static_cast<std::underlying_type_t<Graphics::ImageTexture::ImportMode>>(0u))));
				};
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

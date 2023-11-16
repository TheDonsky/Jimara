#include "FontAssetImporter.h"
#include "../AssetDatabase/FileSystemDatabase/FileSystemDatabase.h"
#include "../Fonts/Freetype/FreetypeFont.h"
#include "../../Core/Memory/RAMBuffer.h"

namespace Jimara {
	namespace {
		class FontAssetSerializer;
		class FreetypeAssetImporter;

		class FreetypeFontAsset : public virtual Asset::Of<Font> {
		private:
			const Reference<const FreetypeAssetImporter> m_importer;

		public:
			inline FreetypeFontAsset(const FreetypeAssetImporter* importer);
			inline virtual ~FreetypeFontAsset();
			inline virtual Reference<Font> LoadItem() override;
		};

		class FreetypeAssetImporter : public virtual FileSystemDatabase::AssetImporter {
		private:
			GUID m_guid = GUID::Generate();

			friend class FontAssetSerializer;
			friend class FreetypeFontAsset;

		public:
			inline virtual bool Import(Callback<const AssetInfo&> reportAsset) override {
				const OS::Path& path = AssetFilePath();
				Reference<OS::MMappedFile> mapping = OS::MMappedFile::Create(path, Log());
				auto fail = [&](const auto&... message) { 
					Log()->Error("FreetypeAssetImporter::Import - ", message...); 
					return false;
				};
				if (mapping == nullptr)
					return fail("Failed to memory map '", path, "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				const Reference<Font> font = FreetypeFont::Create(*mapping, 0u, GraphicsDevice());
				if (font == nullptr)
					return fail("Failed to decode font '", path, "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				
				AssetInfo info;
				info.asset = Object::Instantiate<FreetypeFontAsset>(this);
				info.resourceName = OS::Path(path.filename());
				reportAsset(info);
				return true;
			}
		};

		inline FreetypeFontAsset::FreetypeFontAsset(const FreetypeAssetImporter* importer)
			: Asset(importer->m_guid), m_importer(importer) {}
		inline FreetypeFontAsset::~FreetypeFontAsset() {}
		inline Reference<Font> FreetypeFontAsset::LoadItem() {
			auto fail = [&](const auto&... message) {
				m_importer->Log()->Error("FreetypeFontAsset::LoadItem - ", message...);
				return nullptr;
			};

			const OS::Path path = m_importer->AssetFilePath();
			Reference<OS::MMappedFile> mapping = OS::MMappedFile::Create(path, m_importer->Log());
			if (mapping == nullptr)
				return fail("Failed to memory map '", path, "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			const Reference<RAMBuffer> buffer = Object::Instantiate<RAMBuffer>(mapping->operator MemoryBlock());
			const Reference<Font> font = FreetypeFont::Create(*buffer, 0u, m_importer->GraphicsDevice());
			if (font == nullptr)
				return fail("Failed to decode font '", path, "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			return font;
		}

		class FontAssetSerializer : public virtual FileSystemDatabase::AssetImporter::Serializer {
		public:
			inline FontAssetSerializer() : Serialization::ItemSerializer("FontAssetSerializer") {}

			inline virtual ~FontAssetSerializer() {}

			inline virtual Reference<FileSystemDatabase::AssetImporter> CreateReader() final override {
				return Object::Instantiate<FreetypeAssetImporter>();
			}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, FileSystemDatabase::AssetImporter* target)const final override {
				if (target == nullptr) return;
				FreetypeAssetImporter* importer = dynamic_cast<FreetypeAssetImporter*>(target);
				if (importer == nullptr) {
					target->Log()->Error("FontAssetSerializer::GetFields - Target not of the correct type!");
					return;
				}
				{
					static const Reference<const GUID::Serializer> serializer = Object::Instantiate<GUID::Serializer>("AudioClipGUID");
					recordElement(serializer->Serialize(importer->m_guid));
				}
			}

			static FontAssetSerializer* Instance() {
				static FontAssetSerializer instance;
				return &instance;
			}

			static const std::vector<OS::Path>& Extensions() {
				static const std::vector<OS::Path> extensions = {
					OS::Path(".otf"),
					OS::Path(".ttf")
				};
				return extensions;
			}
		};
	}

	template<> void TypeIdDetails::OnRegisterType<FontAssetImporter>() {
		for (size_t i = 0u; i < FontAssetSerializer::Extensions().size(); i++)
			FontAssetSerializer::Instance()->Register(FontAssetSerializer::Extensions()[i]);
	}
	template<> void TypeIdDetails::OnUnregisterType<FontAssetImporter>() {
		for (size_t i = 0u; i < FontAssetSerializer::Extensions().size(); i++)
			FontAssetSerializer::Instance()->Unregister(FontAssetSerializer::Extensions()[i]);
	}
}

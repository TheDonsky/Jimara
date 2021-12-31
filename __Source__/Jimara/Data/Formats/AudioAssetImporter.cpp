#include "AudioAssetImporter.h"
#include "../AssetDatabase/FileSystemDatabase/FileSystemDatabase.h"
#include "../../Audio/Buffers/WaveBuffer.h"

namespace Jimara {
	namespace {
		class RAMBuffer : public virtual Object {
		private:
			const std::vector<uint8_t> m_data;

		public:
			inline RAMBuffer(const MemoryBlock& blockToCopy) 
				: m_data(reinterpret_cast<const uint8_t*>(blockToCopy.Data()), reinterpret_cast<const uint8_t*>(blockToCopy.Data()) + blockToCopy.Size())  {
				static_assert(sizeof(uint8_t) == 1);
			}

			inline operator MemoryBlock()const {
				return MemoryBlock(m_data.data(), m_data.size(), this);
			}
		};

		class WaveAssetSerializer;
		class WaveAssetImporter;

		class WaveAsset : public virtual Asset::Of<Audio::AudioClip> {
		private:
			const Reference<const WaveAssetImporter> m_importer;

		public:
			inline WaveAsset(const WaveAssetImporter* importer);
			inline virtual Reference<Audio::AudioClip> LoadItem() override;
		};

		class WaveAssetImporter : public virtual FileSystemDatabase::AssetImporter {
		private:
			GUID m_guid = GUID::Generate();
			bool m_streamed = true;

			friend class WaveAssetSerializer;
			friend class WaveAsset;

		public:
			inline virtual bool Import(Callback<const AssetInfo&> reportAsset) override {
				const OS::Path& path = AssetFilePath();
				Reference<Audio::AudioBuffer> waveBuffer = Audio::WaveBuffer(path, Log());
				if (waveBuffer == nullptr) return false;
				{
					AssetInfo info;
					info.asset = Object::Instantiate<WaveAsset>(this);
					reportAsset(info);
				}
				return true;
			}
		};

		inline WaveAsset::WaveAsset(const WaveAssetImporter* importer) : Asset::Of<Audio::AudioClip>(importer->m_guid), m_importer(importer) {}
		inline Reference<Audio::AudioClip> WaveAsset::LoadItem() {
			const OS::Path path = m_importer->AssetFilePath();
			Reference<OS::MMappedFile> mapping = OS::MMappedFile::Create(path, m_importer->Log());
			if (mapping == nullptr) {
				m_importer->Log()->Error("WaveAsset::WaveAsset - Failed to mmap path: '", path, "'!");
				return nullptr;
			}
			else {
				MemoryBlock mappedBlock = *mapping;
				Reference<const RAMBuffer> ramBuffer = Object::Instantiate<RAMBuffer>(mappedBlock);
				Reference<Audio::AudioBuffer> waveBuffer = Audio::WaveBuffer(*ramBuffer, m_importer->Log());
				if (waveBuffer == nullptr) {
					m_importer->Log()->Error("WaveAsset::WaveAsset - Failed to create wave buffer from: '", path, "'!");
					return nullptr;
				}
				else return m_importer->AudioDevice()->CreateAudioClip(waveBuffer, m_importer->m_streamed);
			}
		}

		class WaveAssetSerializer : public virtual FileSystemDatabase::AssetImporter::Serializer {
		public:
			inline WaveAssetSerializer() : Serialization::ItemSerializer("WaveAssetSerializer") {}

			inline virtual Reference<FileSystemDatabase::AssetImporter> CreateReader() final override {
				return Object::Instantiate<WaveAssetImporter>();
			}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, FileSystemDatabase::AssetImporter* target)const final override {
				if (target == nullptr) return;
				WaveAssetImporter* importer = dynamic_cast<WaveAssetImporter*>(target);
				if (importer == nullptr) {
					target->Log()->Error("OBJAssetImporterSerializer::GetFields - Target not of the correct type!");
					return;
				}
				{
					static const Reference<const GUID::Serializer> serializer = Object::Instantiate<GUID::Serializer>("AudioClipGUID");
					recordElement(serializer->Serialize(importer->m_guid));
				}
				{
					static const Reference<const Serialization::ItemSerializer::Of<bool>> serializer = Serialization::ValueSerializer<bool>::Create("Streamed");
					recordElement(serializer->Serialize(importer->m_streamed));
				}
			}

			inline static WaveAssetSerializer* Instance() {
				static const Reference<WaveAssetSerializer> instance = Object::Instantiate<WaveAssetSerializer>();
				return instance;
			}

			inline static const OS::Path& Extension() {
				static const OS::Path format = ".wav";
				return format;
			}
		};
	}

	template<> void TypeIdDetails::OnRegisterType<AudioAssetImporter>() {
		WaveAssetSerializer::Instance()->Register(WaveAssetSerializer::Extension());
	}
	template<> void TypeIdDetails::OnUnregisterType<AudioAssetImporter>() {
		WaveAssetSerializer::Instance()->Unregister(WaveAssetSerializer::Extension());
	}
}


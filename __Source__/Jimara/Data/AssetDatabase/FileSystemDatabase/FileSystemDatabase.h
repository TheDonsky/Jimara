#include "../AssetDatabase.h"
#include "../../../Graphics/GraphicsDevice.h"
#include "../../../Audio/AudioDevice.h"
#include "../../../Core/Helpers.h"
#include "../../../OS/IO/DirectoryChangeObserver.h"
#include <thread>


namespace Jimara {
	class FileSystemDatabase;

	class FileSystemAsset : public virtual Asset {
	public:
		struct FileInfo {
			OS::Path path;
			size_t fileSize = 0;
			uint32_t checksum[32] = {};
		};

		struct EnvironmentInfo {
			Graphics::GraphicsDevice* graphicsDevice = nullptr;
			Audio::AudioDevice* audioDevice = nullptr;
		};

		class Loader : public virtual Object {
		protected:
			virtual Reference<FileSystemAsset> CreateEmpty(GUID guid) = 0;

			virtual Reference<FileSystemAsset> CreateAsset(const FileInfo& fileInfo, EnvironmentInfo environment) = 0;

			virtual bool UpdateAsset(const FileInfo& fileInfo, EnvironmentInfo environment, FileSystemAsset* asset) = 0;

			virtual void GetSubAssets(Callback<Reference<FileSystemAsset>> reportSubAsset) { Unused(reportSubAsset); }

			void Register(const OS::Path& extension);

			void Unregister(const OS::Path& extension);

			friend class FileSystemDatabase;
		};

	private:
		friend class FileSystemDatabase;
	};


	/// <summary>
	/// AssetDatabase based on some working directory
	/// </summary>
	class FileSystemDatabase : public virtual AssetDatabase {
	public:
		/// <summary>
		/// Creates a FileSystemDatabase instance
		/// </summary>
		/// <param name="graphicsDevice"> Graphics device to use </param>
		/// <param name="audioDevice"> Audio device to use </param>
		/// <param name="assetDirectory"> Asset directory to use </param>
		/// <returns> FileSystemDatabase if successful; nullptr otherwise </returns>
		static Reference<FileSystemDatabase> Create(
			Graphics::GraphicsDevice* graphicsDevice,
			Audio::AudioDevice* audioDevice,
			const OS::Path& assetDirectory);

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="graphicsDevice"> Graphics device to use </param>
		/// <param name="audioDevice"> Audio device to use </param>
		/// <param name="assetDirectoryObserver"> DirectoryChangeObserver targetting the directory project's assets are located at </param>
		FileSystemDatabase(Graphics::GraphicsDevice* graphicsDevice, Audio::AudioDevice* audioDevice, OS::DirectoryChangeObserver* assetDirectoryObserver);

		/// <summary> Virtual destructor </summary>
		virtual ~FileSystemDatabase();

		/// <summary>
		/// Finds an asset within the database
		/// Note: The asset may or may not be loaded once returned;
		/// </summary>
		/// <param name="id"> Asset identifier </param>
		/// <returns> Asset reference, if found </returns>
		virtual Reference<Asset> FindAsset(const GUID& id) override;


	private:
		// Graphics device
		const Reference<Graphics::GraphicsDevice> m_graphicsDevice;

		// Audio device
		const Reference<Audio::AudioDevice> m_audioDevice;

		// Asset directory change observer
		const Reference<OS::DirectoryChangeObserver> m_assetDirectoryObserver;

		// Lock for asset collection
		std::mutex m_databaseLock;

		// Asset collection by id
		typedef std::unordered_map<GUID, Reference<Asset>> AssetsByGUID;
		AssetsByGUID m_assetsByGUID;

		// Tries to load, reload or update an Asset from a file
		void ScanFile(const OS::Path& file);

		// Invoked, whenever something happens within the observed directory
		void OnFileSystemChanged(const OS::DirectoryChangeObserver::FileChangeInfo& info);
	};
}

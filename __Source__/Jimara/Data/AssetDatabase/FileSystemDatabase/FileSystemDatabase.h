#include "../AssetDatabase.h"
#include "../../../Graphics/GraphicsDevice.h"
#include "../../../Audio/AudioDevice.h"
#include "../../../Core/Helpers.h"
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
		/// Constructor
		/// </summary>
		/// <param name="graphicsDevice"> Graphics device to use </param>
		/// <param name="audioDevice"> Audio device to use </param>
		/// <param name="assetDirectory"> Asset directory to use </param>
		FileSystemDatabase(
			Graphics::GraphicsDevice* graphicsDevice,
			Audio::AudioDevice* audioDevice,
			const OS::Path& assetDirectory = "Assets");

		/// <summary> Virtual destructor </summary>
		virtual ~FileSystemDatabase();

		/// <summary> Rescans entire directory for new/deleted asssets </summary>
		void RescanAll();

		/// <summary>
		/// Scans given sub-directory
		/// </summary>
		/// <param name="directory"> Some directory under the asset directory </param>
		/// <param name="recurse"> If true, sub-directories will be scanned as well </param>
		void ScanDirectory(const OS::Path& directory, bool recurse = false);

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

		// Asset directory
		OS::Path m_assetDirectory;

		// Lock for asset collection
		std::mutex m_databaseLock;

		// Asset collection by id
		typedef std::unordered_map<GUID, Reference<Asset>> AssetsByGUID;
		AssetsByGUID m_assetsByGUID;

		// This status is set from the destructor to interrupt scanning:
		std::atomic<bool> m_dead = false;

		// Scanning thread
		std::thread m_scanningThread;

		// Rescans entire directory for new/deleted asssets with and option to deliberately 
		// make the process slow and sleep between files to make the load on the system minimal:
		void Rescan(bool slow, const OS::Path& subdirectory, bool recursive);

		// Tries to load, reload or update an Asset from a file
		void ScanFile(const OS::Path& file);
	};
}

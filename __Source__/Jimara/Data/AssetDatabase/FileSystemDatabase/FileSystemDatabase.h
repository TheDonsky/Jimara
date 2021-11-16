#pragma once
#include "../AssetDatabase.h"
#include "../../../Graphics/GraphicsDevice.h"
#include "../../../Audio/AudioDevice.h"


namespace Jimara {
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
			const std::string_view& assetDirectory = "Assets");

		/// <summary> Virtual destructor </summary>
		virtual ~FileSystemDatabase();

		/// <summary> Rescans entire directory for new/deleted asssets </summary>
		void RescanAll();

		/// <summary>
		/// Scans given sub-directory
		/// </summary>
		/// <param name="directory"> Some directory under the asset directory </param>
		/// <param name="recurse"> If true, sub-directories will be scanned as well </param>
		void ScanDirectory(const std::string_view& directory, bool recurse = false);

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
		std::string m_assetDirectory;

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
		void Rescan(bool slow, const std::string_view& subdirectory, bool recursive);
	};
}

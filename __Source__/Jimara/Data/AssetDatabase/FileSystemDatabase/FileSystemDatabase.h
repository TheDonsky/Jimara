#include "../AssetDatabase.h"
#include "../../Serialization/ItemSerializers.h"
#include "../../../Graphics/GraphicsDevice.h"
#include "../../../Audio/AudioDevice.h"
#include "../../../Core/Helpers.h"
#include "../../../OS/IO/MMappedFile.h"
#include "../../../OS/IO/DirectoryChangeObserver.h"
#include <thread>


namespace Jimara {
	/// <summary>
	/// AssetDatabase based on some working directory
	/// </summary>
	class FileSystemDatabase : public virtual AssetDatabase {
	public:
		/// <summary>
		/// Basic application context any asset loader may need
		/// </summary>
		struct Context {
			/// <summary> Graphics device </summary>
			Graphics::GraphicsDevice* graphicsDevice = nullptr;

			/// <summary> Audio device </summary>
			Audio::AudioDevice* audioDevice = nullptr;
		};

		/// <summary> Object, responsible for importing assets from files </summary>
		class AssetReader : public virtual Object {
		public:
			/// <summary>
			/// Information about the source file
			/// </summary>
			struct FileInfo {
				/// <summary> Path to the target file </summary>
				OS::Path path;

				/// <summary> Memory mapping of the file </summary>
				const OS::MMappedFile* memoryMapping = nullptr;

				/// <summary> Basic app context </summary>
				Context dbContext;
			};

			/// <summary>
			/// Imports assets from the file
			/// </summary>
			/// <param name="fileInfo"> Information about a file, storing the assets </param>
			/// <param name="reportAsset"> Whenever the FileReader detects an asset within the file, it should report it's presence through this callback </param>
			/// <returns> True, if the entire file got parsed successfully, false otherwise </returns>
			virtual bool Import(const FileInfo& fileInfo, Callback<Asset*> reportAsset) = 0;

			/// <summary> 
			/// Serializer for AssetReader objects, responsible for their instantiation, serialization and extension registration
			/// </summary>
			class Serializer : public virtual Serialization::SerializerList {
			public:
				/// <summary> Creates a new instance of an AssetReader </summary>
				virtual Reference<AssetReader> Create() = 0;

				/// <summary>
				/// Gives access to sub-serializers/fields
				/// </summary>
				/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
				/// <param name="targetAddr"> Serializer target object address </param>
				virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, AssetReader* target)const = 0;

				/// <summary>
				/// Gives access to sub-serializers/fields
				/// </summary>
				/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
				/// <param name="targetAddr"> Serializer target object address </param>
				virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, void* targetAddr)const final override {
					GetFields(recordElement, reinterpret_cast<AssetReader*>(targetAddr));
				}

				/// <summary>
				/// Ties AssetReader::Serializer to the file extension
				/// </summary>
				/// <param name="extension"> File extension to look out for </param>
				void Register(const OS::Path& extension);

				/// <summary>
				/// Unties AssetReader::Serializer from the file extension
				/// </summary>
				/// <param name="extension"> File extension to ignore </param>
				void Unregister(const OS::Path& extension);
			};
		};


		/// <summary>
		/// Creates a FileSystemDatabase instance
		/// </summary>
		/// <param name="graphicsDevice"> Graphics device to use </param>
		/// <param name="audioDevice"> Audio device to use </param>
		/// <param name="assetDirectory"> Asset directory to use </param>
		/// <param name="importThreadCount"> Limit on the import thead count (at least one will be created) </param>
		/// <returns> FileSystemDatabase if successful; nullptr otherwise </returns>
		static Reference<FileSystemDatabase> Create(
			Graphics::GraphicsDevice* graphicsDevice,
			Audio::AudioDevice* audioDevice,
			const OS::Path& assetDirectory,
			size_t importThreadCount = std::thread::hardware_concurrency());

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="graphicsDevice"> Graphics device to use </param>
		/// <param name="audioDevice"> Audio device to use </param>
		/// <param name="assetDirectoryObserver"> DirectoryChangeObserver targetting the directory project's assets are located at </param>
		/// <param name="importThreadCount"> Limit on the import thead count (at least one will be created) </param>
		FileSystemDatabase(
			Graphics::GraphicsDevice* graphicsDevice, 
			Audio::AudioDevice* audioDevice, 
			OS::DirectoryChangeObserver* assetDirectoryObserver,
			size_t importThreadCount = std::thread::hardware_concurrency());

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

		// Lock for directory observer notifications
		std::mutex m_observerLock;

		// Asset collection by id
		typedef std::unordered_map<GUID, Reference<Asset>> AssetsByGUID;
		AssetsByGUID m_assetsByGUID;

		// Lock for asset collection
		std::mutex m_databaseLock;


		// Basic Asset file information
		struct AssetFileInfo {
			OS::Path filePath;
			std::vector<Reference<AssetReader::Serializer>> serializers;
		};

		// Information about the asset's content
		struct AssetReaderInfo {
			OS::Path filePath;
			Reference<AssetReader::Serializer> serializer;
			Reference<AssetReader> reader;
			std::vector<Reference<Asset>> assets;
		};

		// Path to current AssetReaderInfo mapping
		typedef std::unordered_map<OS::Path, AssetReaderInfo> PathReaderInfo;
		PathReaderInfo m_pathReaders;
		
		// Asset (re)import queue
		typedef std::queue<AssetFileInfo> AssetImportQueue;
		AssetImportQueue m_importQueue;

		// Collection of paths already inside the import queue
		typedef std::unordered_set<OS::Path> QueuedPaths;
		QueuedPaths m_queuedPaths;
		
		// Lock and confition for m_importQueue and m_queuedPaths
		std::mutex m_importQueueLock;
		std::condition_variable m_importAvaliable;
		std::atomic<bool> m_dead = false;

		// Import threads
		std::vector<std::thread> m_importThreads;
		void ImportThread();

		// Queues file parsing
		void QueueFile(AssetFileInfo&& fileInfo);

		// Invoked, whenever something happens within the observed directory
		void OnFileSystemChanged(const OS::DirectoryChangeObserver::FileChangeInfo& info);
	};
}

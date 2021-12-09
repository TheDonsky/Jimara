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
	private:
		struct Context;

	public:
		/// <summary> Object, responsible for importing assets from files </summary>
		class AssetImporter : public virtual Object {
		public:
			/// <summary>
			/// Imports assets from the file
			/// </summary>
			/// <param name="reportAsset"> Whenever the FileReader detects an asset within the file, it should report it's presence through this callback </param>
			/// <returns> True, if the entire file got parsed successfully, false otherwise </returns>
			virtual bool Import(Callback<Asset*> reportAsset) = 0;

			/// <summary> Graphics device </summary>
			Graphics::GraphicsDevice* GraphicsDevice()const;

			/// <summary> Audio device </summary>
			Audio::AudioDevice* AudioDevice()const;

			/// <summary> Current path (may change if file gets moved; therefore, accessing it requires a lock and a deep copy) </summary>
			OS::Path AssetFilePath()const;

			/// <summary> Logger </summary>
			OS::Logger* Log()const;

			/// <summary> 
			/// Serializer for AssetImporter objects, responsible for their instantiation, serialization and extension registration
			/// </summary>
			class Serializer : public virtual Serialization::SerializerList {
			public:
				/// <summary> Creates a new instance of an AssetImporter </summary>
				virtual Reference<AssetImporter> CreateReader() = 0;

				/// <summary>
				/// Gives access to sub-serializers/fields
				/// </summary>
				/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
				/// <param name="targetAddr"> Serializer target object address </param>
				virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, AssetImporter* target)const = 0;

				/// <summary>
				/// Gives access to sub-serializers/fields
				/// </summary>
				/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
				/// <param name="targetAddr"> Serializer target object address </param>
				virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, void* targetAddr)const final override {
					GetFields(recordElement, reinterpret_cast<AssetImporter*>(targetAddr));
				}

				/// <summary>
				/// Ties AssetImporter::Serializer to the file extension
				/// </summary>
				/// <param name="extension"> File extension to look out for </param>
				void Register(const OS::Path& extension);

				/// <summary>
				/// Unties AssetImporter::Serializer from the file extension
				/// </summary>
				/// <param name="extension"> File extension to ignore </param>
				void Unregister(const OS::Path& extension);
			};

		private:
			// File system database is allowed to set the internals
			friend class FileSystemDatabase;

			// Owner; inaccessible, but used for internal error checking
			std::atomic<FileSystemDatabase*> m_owner = nullptr;

			// Context
			Reference<const Context> m_context = nullptr;

			// Current path (may change if file gets moved; therefore, accessing it requires a lock and a deep copy)
			OS::Path m_path;
			mutable std::mutex m_pathLock;
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

		/// <summary> Number of assets currently stored inside the database </summary>
		size_t AssetCount()const;

		/// <summary>
		/// Asset change type
		/// </summary>
		enum class AssetChangeType : uint8_t {
			/// <summary> Nothing happened (never used) </summary>
			NO_CHANGE = 0,

			/// <summary> Asset created/discovered </summary>
			ASSET_CREATED = 1,

			/// <summary> Asset deleted/lost </summary>
			ASSET_DELETED = 2,

			/// <summary> Asset modified </summary>
			ASSET_MODIFIED = 3,

			/// <summary> Not a valid change type; just the number of valid types </summary>
			AssetChangeType_COUNT = 4
		};

		/// <summary>
		/// Information about an asset change within the database
		/// </summary>
		struct DatabaseChangeInfo {
			/// <summary> GUID of the asset in question </summary>
			GUID assetGUID = {};

			/// <summary> Information, about "what happened" to the asset record </summary>
			AssetChangeType changeType;
		};

		/// <summary> Invoked each time the asset database internals change </summary>
		Event<DatabaseChangeInfo>& OnDatabaseChanged()const;


	private:
		// Basic app context
		struct Context : public virtual Object {
			/// Graphics device
			Graphics::GraphicsDevice* graphicsDevice = nullptr;

			// Audio device
			Audio::AudioDevice* audioDevice = nullptr;
		};

		// Basic application context
		const Reference<const Context> m_context;

		// Asset directory change observer
		const Reference<OS::DirectoryChangeObserver> m_assetDirectoryObserver;

		// Lock for directory observer notifications
		std::mutex m_observerLock;

		// Asset collection by id
		typedef std::unordered_map<GUID, Reference<Asset>> AssetsByGUID;
		AssetsByGUID m_assetsByGUID;

		// Lock for asset collection
		mutable std::mutex m_databaseLock;


		// Lock for any given path
		class PathLock : public virtual ObjectCache<OS::Path>::StoredObject, public virtual std::mutex {
		public:
			class Cache : public virtual ObjectCache<OS::Path> {
			public:
				inline Reference<PathLock> LockFor(const OS::Path& path) {
					return GetCachedOrCreate(path, false, [&]()-> Reference<PathLock> {
						return Object::Instantiate<PathLock>();
						});
				}
			};
		};
		PathLock::Cache m_pathLockCache;

		// Basic Asset file information
		struct AssetFileInfo {
			OS::Path filePath;
			std::vector<Reference<AssetImporter::Serializer>> serializers;
		};

		// Information about the asset's content
		struct AssetReaderInfo : public virtual Object {
			Reference<AssetImporter::Serializer> serializer;
			Reference<AssetImporter> reader;
			std::vector<Reference<Asset>> assets;
		};

		// Path to current AssetReaderInfo mapping
		typedef std::unordered_map<OS::Path, Reference<AssetReaderInfo>> PathReaderInfo;
		PathReaderInfo m_pathReaders;
		std::mutex m_pathReaderLock;
		
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

		// Invoked each time the asset database internals change
		mutable EventInstance<DatabaseChangeInfo> m_onDatabaseChanged;

		// Import threads
		std::vector<std::thread> m_importThreads;
		void ImportThread();

		// (Re)Imports the file
		void ImportFile(const AssetFileInfo& fileInfo);

		// Queues file parsing
		void QueueFile(AssetFileInfo&& fileInfo);

		// Invoked, when a file gets renamed; should act accordingly
		void FileRenamed(const OS::Path& oldPath, const OS::Path& newPath);

		// Invoked, when a file gets erased; should act accordingly
		void FileErased(const OS::Path& path);

		// Invoked, whenever something happens within the observed directory
		void OnFileSystemChanged(const OS::DirectoryChangeObserver::FileChangeInfo& info);
	};
}

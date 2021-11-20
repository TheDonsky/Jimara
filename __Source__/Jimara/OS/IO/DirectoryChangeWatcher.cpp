#include "DirectoryChangeWatcher.h"
#include "../../Core/Collections/ObjectCache.h"
#include <thread>
#ifdef _WIN32
#include <windows.h>
#else
#endif


namespace Jimara {
	namespace OS {
		namespace {

			class DirChangeWatcher : public virtual DirectoryChangeWatcher {
			private:
				const Reference<Logger> m_logger;
				const Path m_directory;
				mutable EventInstance<const FileChangeInfo&> m_onFileChanged;

				std::thread m_pollingThread;
				std::thread m_notifyThread;

				std::vector<FileChangeInfo> m_events;
				std::vector<FileChangeInfo> m_eventBuffer;
				std::mutex m_eventLock;
				std::atomic<bool> m_dead = false;

				inline void QueueEvent(FileChangeInfo&& event) {
					std::unique_lock<std::mutex> lock(m_eventLock);
					m_events.push_back(std::move(event));
				}

				inline void Notify() {
					{
						std::unique_lock<std::mutex> lock(m_eventLock);
						std::swap(m_events, m_eventBuffer);
					}
					for (size_t i = 0; i < m_eventBuffer.size(); i++)
						m_onFileChanged(m_eventBuffer[i]);
					m_eventBuffer.clear();
				}

				inline void Poll();

				inline void StartThreads() {
					static void(*thread)(Callback<>, const std::atomic<bool>*) = [](Callback<> update, const std::atomic<bool>* dead) {
						while (!dead->load()) update();
					};
					m_pollingThread = std::thread(thread, Callback<>(&DirChangeWatcher::Poll, this), &m_dead);
					m_notifyThread = std::thread(thread, Callback<>(&DirChangeWatcher::Notify, this), &m_dead);
				}

				inline void KillThreads() {
					if (m_dead) return;
					m_dead = true;
					m_pollingThread.join();
					m_notifyThread.join();
				}

#ifdef _WIN32
				HANDLE m_directoryHandle = INVALID_HANDLE_VALUE;
				alignas(DWORD) uint8_t m_notifyInfoBuffer[16777216]; // Excessive? maybe... but unlikely to evet oveflow, I think...

				inline DirChangeWatcher(const Path& directory, HANDLE directoryHandle);
#else
#endif

			public:
				inline virtual ~DirChangeWatcher();

				virtual const Path& Directory()const final override { return m_directory; }

				inline virtual Event<const FileChangeInfo&>& OnFileChanged()const final override { return m_onFileChanged; }

				inline static Reference<DirChangeWatcher> Open(const Path& directory, OS::Logger* logger);
			};

#ifdef _WIN32
			inline void DirChangeWatcher::Poll() {
				auto pollDirectory = [&](const Path& dirPath, HANDLE dirHandle) {
					DWORD numBytesReturned = 0;
					BOOL rv = ReadDirectoryChangesW(
						dirHandle,
						m_notifyInfoBuffer, sizeof(m_notifyInfoBuffer),
						true, // We watch entire subtree...
						FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
						&numBytesReturned, 
						NULL, NULL);
					if (rv != 0) {
						if (m_logger != nullptr) m_logger->Error(
							"DirChangeWatcher::Poll - ReadDirectoryChangesW failed for '", dirPath, "' failed with error code ", GetLastError(), "!");
						return;
					}
					// __TODO__: Interpret results...
				};
				pollDirectory(m_directory, m_directoryHandle);
			}

			inline DirChangeWatcher::DirChangeWatcher(const Path& directory, HANDLE directoryHandle) 
				: m_directory(directory), m_directoryHandle(directoryHandle) {
				StartThreads();
			}

			inline DirChangeWatcher::~DirChangeWatcher() {
				KillThreads();
				{
					BOOL rv = CloseHandle(m_directoryHandle);
					if (m_logger != nullptr && rv == 0)
						m_logger->Error("DirChangeWatcher::~DirChangeWatcher - CloseHandle(m_directoryHandle) Failed! <rv=", rv, ">");
				}
			}

			inline Reference<DirChangeWatcher> DirChangeWatcher::Open(const Path& directory, OS::Logger* logger) {
				HANDLE directoryHandle = [&]() -> HANDLE {
					const std::wstring path = directory;
					HANDLE handle = CreateFileW(
						path.c_str(), FILE_LIST_DIRECTORY,
						FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE, NULL,
						OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
					if (handle == INVALID_HANDLE_VALUE && logger != nullptr)
						logger->Error("DirectoryChangeWatcher::Create - CreateFileW() failed for '", directory, "'!");
					return handle;
				}();
				if (directoryHandle == INVALID_HANDLE_VALUE) return nullptr;

				if (logger != nullptr) logger->Error("DirectoryChangeWatcher::Create - OS Not(yet) Supported! (attempting to open for '", directory, "')");
				return nullptr;
			}



#else
			inline void DirChangeWatcher::Poll() {

			}

			inline DirChangeWatcher::DirChangeWatcher(const Path& directory)
				: m_directory(directory) {
				StartThreads();
				// __TODO__: Deal with polling thread
			}

			inline DirChangeWatcher::~DirChangeWatcher() {
				KillThreads();
				// __TODO__: Close any handles if needed
			}

			inline Reference<DirChangeWatcher> DirChangeWatcher::Open(const Path& directory, OS::Logger* logger) {
				if (logger != nullptr) logger->Error("DirectoryChangeWatcher::Create - OS Not(yet) Supported! (attempting to open for '", directory, "')");
				return nullptr;
			}
#endif

			class DirChangeWatcherCache : ObjectCache<Path> {
			private:
#pragma warning(disable: 4250)
				class Cached : public virtual DirectoryChangeWatcher, public virtual ObjectCache<Path>::StoredObject {
				private:
					const Reference<DirectoryChangeWatcher> m_base;
				public:
					inline Cached(DirectoryChangeWatcher* base) : m_base(base) {}
					virtual const Path& Directory()const final override { return m_base->Directory(); }
					inline virtual Event<const FileChangeInfo&>& OnFileChanged()const final override { return m_base->OnFileChanged(); }
				};
#pragma warning(default: 4250)

			public:
				static Reference<DirChangeWatcher> Open(const Path& directory, OS::Logger* logger) {
					static DirChangeWatcherCache cache;
					return cache.GetCachedOrCreate(directory, false, [&]()->Reference<DirChangeWatcher> {
						const Reference<DirectoryChangeWatcher> base = DirChangeWatcher::Open(directory, logger);
						if (base == nullptr) return nullptr;
						else return Object::Instantiate<Cached>(base);
						});
				}
			};
		}


		Reference<DirectoryChangeWatcher> DirectoryChangeWatcher::Create(const Path& directory, OS::Logger* logger, bool cached) {
			if (cached) return DirChangeWatcherCache::Open(directory, logger);
			else return DirChangeWatcher::Open(directory, logger);
		}
	}
}

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
#ifdef _WIN32
			class DirectoryListener : public virtual Object {
			private:
				const Path m_directory;
				const Reference<OS::Logger> m_logger;
				HANDLE m_directoryHandle = INVALID_HANDLE_VALUE;
				OVERLAPPED m_overlapped = {};
				alignas(DWORD) uint8_t m_notifyInfoBuffer[1 << 20] = {}; // Excessive? I have no idea...
				bool m_scheduled = false;

				inline DirectoryListener(const DirectoryListener&) = delete;
				inline DirectoryListener(DirectoryListener&&) = delete;
				inline DirectoryListener& operator=(const DirectoryListener&) = delete;
				inline DirectoryListener& operator=(DirectoryListener&&) = delete;

			public:
				enum class Action : uint8_t {
					NONE = 0,
					ADDED = 1,
					REMOVED = 2,
					MODIFIED = 3,
					RENAMED_OLD_NAME = 4,
					RENAMED_NEW_NAME = 5
				};

				struct FileUpdate {
					Reference<DirectoryListener> listener;
					Action action = Action::NONE;
					Path filePath;
				};

				inline DirectoryListener(const Path& path, OS::Logger* log) : m_directory(path), m_logger(log) {}

				inline static Reference<DirectoryListener> Create(const Path& directory, OS::Logger* logger) {
					Reference<DirectoryListener> result = Object::Instantiate<DirectoryListener>(directory, logger);

					result->m_directoryHandle = [&]() -> HANDLE {
						const std::wstring path = directory;
						HANDLE handle = CreateFileW(
							path.c_str(), FILE_LIST_DIRECTORY,
							FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE, NULL,
							OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
						if (handle == INVALID_HANDLE_VALUE)
							logger->Error("(DirectoryChangeWatcher::)DirectoryListener::Create - CreateFileW(\"", directory, "\") failed with code: ", GetLastError(), "!");
						return handle;
					}();
					if (result->m_directoryHandle == INVALID_HANDLE_VALUE) return nullptr;

					result->m_overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
					if (result->m_overlapped.hEvent == NULL) {
						logger->Error("(DirectoryChangeWatcher::)DirectoryListener::Create - CreateEvent() failed for directory: '", directory, "'; error code: ", GetLastError(), "!");
						return nullptr;
					}

					return result;
				}

				inline virtual ~DirectoryListener() {
					auto closeHandle = [&](HANDLE& handle, HANDLE nullValue, const char* handleName) {
						if (handle == nullValue) return;
						else if (CloseHandle(handle) == 0)
							m_logger->Error("(DirectoryChangeWatcher::)DirectoryListener::Destroy - CloseHandle(", handleName, ") Failed! <rv=", GetLastError(), ">; handle leaked...");
						handle = nullValue;
					};
					closeHandle(m_directoryHandle, INVALID_HANDLE_VALUE, "directoryHandle");
					closeHandle(m_overlapped.hEvent, NULL, "overlapped.hEvent");
				}

				template<typename EmitResult>
				inline void Refresh(DWORD timeout, const EmitResult& emitResult) {
					auto scheduleRead = [&]() {
						if (m_scheduled) return true;
						else if (ReadDirectoryChangesW(
							m_directoryHandle,
							m_notifyInfoBuffer,
							sizeof(m_notifyInfoBuffer),
							TRUE,
							FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
							NULL, &m_overlapped, NULL) == 0) {
							m_logger->Error("(DirectoryChangeWatcher::)DirectoryListener::ScheduleRead - Failed to schedule read using ReadDirectoryChangesW! error code: ", GetLastError(), ";");
						}
						else m_scheduled = true;
						return m_scheduled;
					};

					if (!scheduleRead()) return;
					DWORD waitResult = WaitForSingleObject(m_overlapped.hEvent, timeout);
					if (waitResult == WAIT_ABANDONED)
						m_logger->Error("(DirectoryChangeWatcher::)DirectoryListener::Refresh - Got 'WAIT_ABANDONED' for '", m_directory, "' <internal error>!");
					else if (waitResult == WAIT_OBJECT_0) {
						// __TODO__: Read and generate signal...
						DWORD numberOfBytesTransferred = 0;
						if (GetOverlappedResult(m_directoryHandle, &m_overlapped, &numberOfBytesTransferred, FALSE) == 0)
							m_logger->Error("(DirectoryChangeWatcher::)DirectoryListener::Refresh - GetOverlappedResult '", m_directory, "' Failed! (error code: ", GetLastError(), ")");
						else {
							DWORD bytesInspected = 0;
							while (bytesInspected < numberOfBytesTransferred) {
								const FILE_NOTIFY_INFORMATION* notifyInfo = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(m_notifyInfoBuffer + bytesInspected);
								
								if ((notifyInfo->FileNameLength % sizeof(wchar_t)) != 0) {
									m_logger->Error("(DirectoryChangeWatcher::)DirectoryListener::Refresh - Invalid name length!");
									break;
								}
								else if (notifyInfo->NextEntryOffset > (offsetof(FILE_NOTIFY_INFORMATION, FileName) + notifyInfo->FileNameLength)) {
									m_logger->Error("(DirectoryChangeWatcher::)DirectoryListener::Refresh - Name length overflow!");
									break;
								}

								FileUpdate update;
								update.listener = this;
								update.filePath = [&]() -> Path {
									std::wstring filePath;
									filePath.resize(notifyInfo->FileNameLength / sizeof(wchar_t));
									const wchar_t* ptr = notifyInfo->FileName;
									for (size_t i = 0; i < filePath.size(); i++)
										filePath[i] = ptr[i];
									return filePath;
								}();
								update.action =
									(notifyInfo->Action == FILE_ACTION_ADDED) ? Action::ADDED :
									(notifyInfo->Action == FILE_ACTION_REMOVED) ? Action::REMOVED :
									(notifyInfo->Action == FILE_ACTION_MODIFIED) ? Action::MODIFIED :
									(notifyInfo->Action == FILE_ACTION_RENAMED_OLD_NAME) ? Action::RENAMED_OLD_NAME :
									(notifyInfo->Action == FILE_ACTION_RENAMED_NEW_NAME) ? Action::RENAMED_NEW_NAME : Action::NONE;
								if (update.action != Action::NONE)
									emitResult(std::move(update));
								else m_logger->Warning("(DirectoryChangeWatcher::)DirectoryListener::Refresh - '", m_directory, "' got unknown action for file '", update.filePath, "'!");
								
								if (notifyInfo->NextEntryOffset == 0) break;
								else bytesInspected += notifyInfo->NextEntryOffset;
							}
						}
						ResetEvent(m_overlapped.hEvent);
						m_scheduled = false;
						scheduleRead();
					}
					else if (waitResult == WAIT_TIMEOUT) {} // Good... nothing has to happen in this case...
					else if (waitResult == WAIT_FAILED)
						m_logger->Error("(DirectoryChangeWatcher::)DirectoryListener::Refresh - Got 'WAIT_FAILED' for '", m_directory, "'! (error code: ", GetLastError(), ")");
				}
				inline operator const Path& ()const { return m_directory; }
				inline operator bool()const { return m_directoryHandle != INVALID_HANDLE_VALUE && m_overlapped.hEvent != NULL; }
			};

			class SymlinkOverlaps {
			private:
				typedef std::unordered_map<Path, Reference<DirectoryListener>> SymlinkListeners;
				//typedef std::unordered_map<HANDLE, Path> OverlapEventToPath;

				SymlinkListeners m_dirListeners;
				//OverlapEventToPath eventToPath;

			public:
				inline void Add(const Path& path, const Path& rootPath, OS::Logger* logger) {
					const Path absPath = std::filesystem::absolute(path);
					if (absPath == std::filesystem::absolute(rootPath)) return;
					else if (m_dirListeners.find(absPath) != m_dirListeners.end()) return;
					Reference<DirectoryListener> overlap = DirectoryListener::Create(path, logger);
					if (overlap != nullptr) {
						m_dirListeners[absPath] = overlap;
						//eventToPath[overlap->overlapped.hEvent] = absPath;
					}
				}

				inline void Remove(const Path& path) {
					const Path absPath = std::filesystem::absolute(path);
					SymlinkListeners::iterator it = m_dirListeners.find(absPath);
					if (it == m_dirListeners.end()) return;
					//eventToPath.erase(it->second->overlapped.hEvent);
					m_dirListeners.erase(it);
				}

				inline void Clear() {
					//eventToPath.clear();
					m_dirListeners.clear();
				}

				template<typename EmitResult>
				inline void Refresh(const EmitResult& emitResult) {
					for (SymlinkListeners::const_iterator it = m_dirListeners.begin(); it != m_dirListeners.end(); ++it)
						it->second->Refresh(0, emitResult);
				}
			};
#else
#endif

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
				Reference<DirectoryListener> m_rootListener;
				SymlinkOverlaps m_symlinkListeners;

				inline DirChangeWatcher(DirectoryListener* rootListener);


#else
				inline DirChangeWatcher(const Path& directory);
#endif

			public:
				inline virtual ~DirChangeWatcher();

				virtual const Path& Directory()const final override { return m_directory; }

				inline virtual Event<const FileChangeInfo&>& OnFileChanged()const final override { return m_onFileChanged; }

				inline static Reference<DirChangeWatcher> Open(const Path& directory, OS::Logger* logger);
			};

#ifdef _WIN32
			inline void DirChangeWatcher::Poll() {
				auto onEvent = [&](const DirectoryListener::FileUpdate& update) {
					// __TODO__: Interpret what happened in some way...
				};
				m_rootListener->Refresh(1, onEvent);
				m_symlinkListeners.Refresh(onEvent);
				// __TODO__: Do something about anything....
			}

			inline DirChangeWatcher::DirChangeWatcher(DirectoryListener* rootListener)
				: m_directory(*rootListener), m_rootListener(rootListener) {
				Path::IterateDirectory(m_directory, [&](const Path& subpath) {
					if (std::filesystem::is_symlink(subpath))
						m_symlinkListeners.Add(subpath, m_directory, m_logger);
					return true;
					}, Path::IterateDirectoryFlags::REPORT_DIRECTORIES_RECURSIVE);
				StartThreads();
			}

			inline DirChangeWatcher::~DirChangeWatcher() {
				KillThreads();
				m_symlinkListeners.Clear();
				m_rootListener = nullptr;
			}

			inline Reference<DirChangeWatcher> DirChangeWatcher::Open(const Path& directory, OS::Logger* logger) {
				Reference<DirectoryListener> rootDirectoryListener = DirectoryListener::Create(directory, logger);
				if (rootDirectoryListener == nullptr) {
					logger->Error("DirectoryChangeWatcher::Create - Failed to start listening to '", directory, "'!");
					return nullptr;
				}
				else {
					Reference<DirChangeWatcher> watcher = new DirChangeWatcher(rootDirectoryListener);
					watcher->ReleaseRef();
					return watcher;
				}
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
				static Reference<DirectoryChangeWatcher> Open(const Path& directory, OS::Logger* logger) {
					static DirChangeWatcherCache cache;
					return cache.GetCachedOrCreate(directory, false, [&]()->Reference<DirectoryChangeWatcher> {
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

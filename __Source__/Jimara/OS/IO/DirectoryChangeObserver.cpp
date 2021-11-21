#include "DirectoryChangeObserver.h"
#include "../../Core/Collections/ObjectCache.h"
#include <condition_variable>
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
			public:
				typedef DirectoryChangeObserver::FileChangeInfo FileUpdate;

			private:
				const Path m_directory;
				const Reference<OS::Logger> m_logger;
				HANDLE m_directoryHandle = INVALID_HANDLE_VALUE;
				OVERLAPPED m_overlapped = {};
				alignas(DWORD) uint8_t m_notifyInfoBuffer[1 << 20] = {}; // Excessive? I have no idea...
				bool m_scheduled = false;
				std::optional<FileUpdate> m_fileMovedOldFile;

				inline DirectoryListener(const DirectoryListener&) = delete;
				inline DirectoryListener(DirectoryListener&&) = delete;
				inline DirectoryListener& operator=(const DirectoryListener&) = delete;
				inline DirectoryListener& operator=(DirectoryListener&&) = delete;

			public:
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
								else if (
									((notifyInfo->NextEntryOffset != 0) ? notifyInfo->NextEntryOffset : (numberOfBytesTransferred - bytesInspected))
									< (offsetof(FILE_NOTIFY_INFORMATION, FileName) + notifyInfo->FileNameLength)) {
									m_logger->Error("(DirectoryChangeWatcher::)DirectoryListener::Refresh - Name length overflow!");
									break;
								}

								FileUpdate update;
								update.filePath = [&]() -> Path {
									std::wstring filePath;
									filePath.resize(notifyInfo->FileNameLength / sizeof(wchar_t));
									const wchar_t* ptr = notifyInfo->FileName;
									for (size_t i = 0; i < filePath.size(); i++)
										filePath[i] = ptr[i];
									return m_directory / filePath;
								}();
								if (notifyInfo->Action == FILE_ACTION_RENAMED_OLD_NAME) {
									if (m_fileMovedOldFile.has_value()) {
										FileUpdate& removedFile = m_fileMovedOldFile.value();
										removedFile.changeType = DirectoryChangeObserver::FileChangeType::DELETED;
										emitResult(std::move(removedFile)); // I guess, it moved somewhere outside our jurisdiction...
									}
									m_fileMovedOldFile = update;
								}
								else if (notifyInfo->Action == FILE_ACTION_RENAMED_NEW_NAME) {
									if (m_fileMovedOldFile.has_value()) {
										update.oldPath = m_fileMovedOldFile.value().filePath;
										update.changeType = DirectoryChangeObserver::FileChangeType::RENAMED;
										m_fileMovedOldFile = std::optional<FileUpdate>();
										assert(!m_fileMovedOldFile.has_value());
									}
									else update.changeType = DirectoryChangeObserver::FileChangeType::CREATED; // I guess, it moved from somewhere outside our jurisdiction...
									emitResult(std::move(update));
								}
								else {
									update.changeType =
										(notifyInfo->Action == FILE_ACTION_ADDED) ? DirectoryChangeObserver::FileChangeType::CREATED :
										(notifyInfo->Action == FILE_ACTION_REMOVED) ? DirectoryChangeObserver::FileChangeType::DELETED :
										(notifyInfo->Action == FILE_ACTION_MODIFIED) ? DirectoryChangeObserver::FileChangeType::CHANGED : DirectoryChangeObserver::FileChangeType::NO_OP;
									if (update.changeType != DirectoryChangeObserver::FileChangeType::NO_OP)
										emitResult(std::move(update));
									else m_logger->Warning(
										"(DirectoryChangeWatcher::)DirectoryListener::Refresh - '", m_directory, "' got unknown action for file '", update.filePath, "'!");
								}
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
				SymlinkListeners m_dirListeners;

			public:
				inline void Add(const Path& path, const Path& rootPath, OS::Logger* logger) {
					const Path absPath = std::filesystem::absolute(path);
					if (absPath == std::filesystem::absolute(rootPath)) return;
					else if (m_dirListeners.find(absPath) != m_dirListeners.end()) return;
					Reference<DirectoryListener> listener = DirectoryListener::Create(path, logger);
					if (listener != nullptr)
						m_dirListeners[absPath] = listener;
				}

				inline void Remove(const Path& path) {
					{
						const Path absPath = std::filesystem::absolute(path);
						SymlinkListeners::iterator it = m_dirListeners.find(absPath);
						if (it == m_dirListeners.end()) return;
					}
					std::vector<Path> subListeners;
					const std::wstring pathString = ((std::wstring)path) + L"/";
					for (SymlinkListeners::const_iterator it = m_dirListeners.begin(); it != m_dirListeners.end(); ++it) {
						const std::wstring subPath = it->second->operator const Jimara::OS::Path &();
						if (subPath.length() < pathString.length()) continue;
						bool isSubstr = true;
						for (size_t i = 0; i < pathString.length(); i++)
							if (pathString[i] != subPath[i]) {
								isSubstr = false;
								break;
							}
						if (isSubstr)
							subListeners.push_back(it->first);
					}
					for (size_t i = 0; i < subListeners.size(); i++)
						m_dirListeners.erase(subListeners[i]);
				}

				inline void Clear() { m_dirListeners.clear(); }

				template<typename EmitResult>
				inline void Refresh(const EmitResult& emitResult) {
					for (SymlinkListeners::const_iterator it = m_dirListeners.begin(); it != m_dirListeners.end(); ++it)
						it->second->Refresh(0, emitResult);
				}
			};
#else
#endif

			class DirChangeWatcher : public virtual DirectoryChangeObserver {
			private:
				const Reference<Logger> m_logger;
				const Path m_directory;
				mutable EventInstance<const FileChangeInfo&> m_onFileChanged;

				std::thread m_pollingThread;
				std::thread m_notifyThread;

				std::vector<FileChangeInfo> m_events;
				std::vector<FileChangeInfo> m_eventBuffer;
				std::mutex m_eventQueueLock;
				std::condition_variable m_eventQueueCondition;
				std::atomic<bool> m_dead = false;

				inline void QueueEvent(FileChangeInfo&& event) {
					std::lock_guard<std::mutex> lock(m_eventQueueLock);
					m_events.push_back(std::move(event));
					m_eventQueueCondition.notify_all();
				}

				inline void Notify() {
					{
						std::unique_lock<std::mutex> lock(m_eventQueueLock);
						while (m_events.empty() && (!m_dead)) m_eventQueueCondition.wait(lock);
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
					{
						std::lock_guard<std::mutex> lock(m_eventQueueLock);
						m_dead = true;
						m_eventQueueCondition.notify_all();
					}
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
				std::vector<Path> removedLinks;
				std::vector<Path> addedLinks;
				auto onEvent = [&](DirectoryListener::FileUpdate&& update) {
					update.observer = this;
					if (std::filesystem::is_symlink(update.filePath)) {
						if (update.changeType == FileChangeType::CREATED)
							addedLinks.push_back(update.filePath);
						else if (update.changeType == FileChangeType::DELETED)
							removedLinks.push_back(update.filePath);
						else if (update.changeType == FileChangeType::RENAMED) {
							addedLinks.push_back(update.filePath);
							removedLinks.push_back(update.oldPath.value());
						}
						else if (update.changeType == FileChangeType::CHANGED) {
							removedLinks.push_back(update.filePath);
							addedLinks.push_back(update.filePath);
						}
					}
					QueueEvent(std::move(update));
				};
				m_rootListener->Refresh(1, onEvent);
				m_symlinkListeners.Refresh(onEvent);

				for (size_t i = 0; i < removedLinks.size(); i++)
					m_symlinkListeners.Remove(removedLinks[i]);

				for (size_t i = 0; i < addedLinks.size(); i++)
					Path::IterateDirectory(addedLinks[i], [&](const Path& subpath) {
					if (std::filesystem::is_symlink(subpath))
						m_symlinkListeners.Add(subpath, m_directory, m_logger);
					return true;
						}, Path::IterateDirectoryFlags::REPORT_DIRECTORIES_RECURSIVE);
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
				class Cached : public virtual DirectoryChangeObserver, public virtual ObjectCache<Path>::StoredObject {
				private:
					const Reference<DirectoryChangeObserver> m_base;
				public:
					inline Cached(DirectoryChangeObserver* base) : m_base(base) {}
					virtual const Path& Directory()const final override { return m_base->Directory(); }
					inline virtual Event<const FileChangeInfo&>& OnFileChanged()const final override { return m_base->OnFileChanged(); }
				};
#pragma warning(default: 4250)

			public:
				static Reference<DirectoryChangeObserver> Open(const Path& directory, OS::Logger* logger) {
					static DirChangeWatcherCache cache;
					return cache.GetCachedOrCreate(directory, false, [&]()->Reference<DirectoryChangeObserver> {
						const Reference<DirectoryChangeObserver> base = DirChangeWatcher::Open(directory, logger);
						if (base == nullptr) return nullptr;
						else return Object::Instantiate<Cached>(base);
						});
				}
			};
		}


		Reference<DirectoryChangeObserver> DirectoryChangeObserver::Create(const Path& directory, OS::Logger* logger, bool cached) {
			if (cached) return DirChangeWatcherCache::Open(directory, logger);
			else return DirChangeWatcher::Open(directory, logger);
		}
	}
}

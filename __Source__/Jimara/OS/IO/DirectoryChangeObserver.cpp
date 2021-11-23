#include "DirectoryChangeObserver.h"
#include "../../Core/Collections/ObjectCache.h"
#include <condition_variable>
#include <thread>
#include <set>
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
				const Path m_absolutePath;
				Path m_mainAlias;
				std::set<Path> m_aliases;
				const Reference<OS::Logger> m_logger;
				HANDLE m_directoryHandle = INVALID_HANDLE_VALUE;
				OVERLAPPED m_overlapped = {};
				alignas(DWORD) uint8_t m_notifyInfoBuffer[1 << 20] = {}; // Excessive? I have no idea...
				bool m_scheduled = false;
				std::optional<FileUpdate> m_fileMovedOldFile;
				std::unordered_set<Path> m_allFilesRelative;

				inline DirectoryListener(const DirectoryListener&) = delete;
				inline DirectoryListener(DirectoryListener&&) = delete;
				inline DirectoryListener& operator=(const DirectoryListener&) = delete;
				inline DirectoryListener& operator=(DirectoryListener&&) = delete;

				inline bool FindAllFiles(const Path& subPath) {
					{
						std::error_code error;
						const Path relPath = std::filesystem::relative(subPath, m_absolutePath, error);
						if (error) return true;
						m_allFilesRelative.insert(relPath);
					}
					{
						std::error_code error;
						bool symlink = std::filesystem::is_symlink(subPath, error);
						if (symlink || error) return true;
					}
					{
						std::error_code error;
						bool directory = std::filesystem::is_directory(subPath, error);
						if ((!directory) || error) return true;
					}
					Path::IterateDirectory(subPath, Function<bool, const Path&>(&DirectoryListener::FindAllFiles, this), Path::IterateDirectoryFlags::REPORT_ALL);
					return true;
				}

			public:
				inline Logger* Log()const { return m_logger; }

				inline const Path& AbsolutePath()const { return m_absolutePath; }

				inline const Path& MainAlias()const { return m_mainAlias; }

				inline bool HasAlias()const { return !m_aliases.empty(); }

				inline void AddAlias(const Path& alias) {
					if (alias == m_absolutePath) return;
					else if (m_aliases.empty()) m_mainAlias = alias;
					m_aliases.insert(alias);
				}

				inline void RemoveAlias(const Path& alias) {
					std::set<Path>::const_iterator it = m_aliases.find(alias);
					if (it == m_aliases.end()) return;
					m_aliases.erase(it);
					if (m_aliases.empty()) m_mainAlias = m_absolutePath;
					else if (m_mainAlias == alias) m_mainAlias = *m_aliases.begin();
				}

				inline DirectoryListener(const Path& absPath, const Path& alias, OS::Logger* log) : m_absolutePath(absPath), m_mainAlias(absPath), m_logger(log) {
					AddAlias(alias);
					Path::IterateDirectory(absPath, Function<bool, const Path&>(&DirectoryListener::FindAllFiles, this), Path::IterateDirectoryFlags::REPORT_ALL);
				}

				inline static Reference<DirectoryListener> Create(const Path& absPath, const Path& alias, OS::Logger* logger) {
					Reference<DirectoryListener> result = Object::Instantiate<DirectoryListener>(absPath, alias, logger);

					result->m_directoryHandle = [&]() -> HANDLE {
						const std::wstring path = absPath;
						HANDLE handle = CreateFileW(
							path.c_str(), FILE_LIST_DIRECTORY,
							FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE, NULL,
							OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
						if (handle == INVALID_HANDLE_VALUE)
							logger->Error("(DirectoryChangeWatcher::)DirectoryListener::Create - CreateFileW(\"", absPath, "\") failed with code: ", GetLastError(), "!");
						return handle;
					}();
					if (result->m_directoryHandle == INVALID_HANDLE_VALUE) return nullptr;

					result->m_overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
					if (result->m_overlapped.hEvent == NULL) {
						logger->Error("(DirectoryChangeWatcher::)DirectoryListener::Create - CreateEvent() failed for directory: '", absPath, "'; error code: ", GetLastError(), "!");
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
						m_logger->Error("(DirectoryChangeWatcher::)DirectoryListener::Refresh - Got 'WAIT_ABANDONED' for '", m_absolutePath, "' <internal error>!");
					else if (waitResult == WAIT_OBJECT_0) {
						DWORD numberOfBytesTransferred = 0;
						if (GetOverlappedResult(m_directoryHandle, &m_overlapped, &numberOfBytesTransferred, FALSE) == 0)
							m_logger->Error("(DirectoryChangeWatcher::)DirectoryListener::Refresh - GetOverlappedResult '", m_absolutePath, "' Failed! (error code: ", GetLastError(), ")");
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
								auto emitUpdate = [&]() {
									update.filePath = MainAlias() / update.filePath;
									emitResult(std::move(update));
								};
								update.filePath = [&]() -> Path {
									std::wstring filePath;
									filePath.resize(notifyInfo->FileNameLength / sizeof(wchar_t));
									const wchar_t* ptr = notifyInfo->FileName;
									for (size_t i = 0; i < filePath.size(); i++)
										filePath[i] = ptr[i];
									return filePath;
								}();
								if (notifyInfo->Action == FILE_ACTION_RENAMED_OLD_NAME) {
									if (m_fileMovedOldFile.has_value()) {
										FileUpdate& removedFile = m_fileMovedOldFile.value();
										removedFile.changeType = DirectoryChangeObserver::FileChangeType::DELETED;
										removedFile.filePath = MainAlias() / removedFile.filePath;
										emitResult(std::move(removedFile)); // I guess, it moved somewhere outside our jurisdiction...
									}
									m_fileMovedOldFile = update;
								}
								else if (notifyInfo->Action == FILE_ACTION_RENAMED_NEW_NAME) {
									if (m_fileMovedOldFile.has_value()) {
										update.oldPath = MainAlias() / m_fileMovedOldFile.value().filePath;
										update.changeType = DirectoryChangeObserver::FileChangeType::RENAMED;
										m_fileMovedOldFile = std::optional<FileUpdate>();
										assert(!m_fileMovedOldFile.has_value());
									}
									else update.changeType = DirectoryChangeObserver::FileChangeType::CREATED; // I guess, it moved from somewhere outside our jurisdiction...
									emitUpdate();
								}
								else {
									update.changeType =
										(notifyInfo->Action == FILE_ACTION_ADDED) ? DirectoryChangeObserver::FileChangeType::CREATED :
										(notifyInfo->Action == FILE_ACTION_REMOVED) ? DirectoryChangeObserver::FileChangeType::DELETED :
										(notifyInfo->Action == FILE_ACTION_MODIFIED) ? DirectoryChangeObserver::FileChangeType::CHANGED : DirectoryChangeObserver::FileChangeType::NO_OP;
									if (update.changeType != DirectoryChangeObserver::FileChangeType::NO_OP) {
										if (update.changeType == DirectoryChangeObserver::FileChangeType::CREATED)
											m_allFilesRelative.insert(update.filePath);
										else if (update.changeType == DirectoryChangeObserver::FileChangeType::DELETED)
											m_allFilesRelative.erase(update.filePath);
										emitUpdate();
									}
									else m_logger->Warning(
										"(DirectoryChangeWatcher::)DirectoryListener::Refresh - '", m_absolutePath, "' got unknown action for file '", update.filePath, "'!");
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
						m_logger->Error("(DirectoryChangeWatcher::)DirectoryListener::Refresh - Got 'WAIT_FAILED' for '", m_absolutePath, "'! (error code: ", GetLastError(), ")");
				}

				template<typename MapFunction>
				inline void ForAllFiles(const MapFunction& callback) {
					for (std::unordered_set<Path>::const_iterator it = m_allFilesRelative.begin(); it != m_allFilesRelative.end(); ++it)
						callback(MainAlias() / (*it));
				}

				inline operator bool()const { return m_directoryHandle != INVALID_HANDLE_VALUE && m_overlapped.hEvent != NULL; }
			};

			class SymlinkOverlaps {
			private:
				typedef std::unordered_map<Path, Reference<DirectoryListener>> SymlinkListeners;
				SymlinkListeners m_dirListeners;
				SymlinkListeners m_aliasedListeners;

			public:
				inline bool ListeningTo(const Path& path) { return m_aliasedListeners.find(path) != m_aliasedListeners.end(); }

				template<typename OnAddedCallback>
				inline void Add(const Path& path, const Path& rootPathAbs, OS::Logger* logger, const OnAddedCallback& onAdded) {
					Path absPath;
					{
						std::error_code error;
						const Path relPath = std::filesystem::relative(path, rootPathAbs, error);
						if (error || relPath.empty()) return; // No support for out-of-partition links...
						absPath = std::filesystem::absolute(rootPathAbs / relPath, error);
						if (error) return;
					}

					if (absPath == rootPathAbs) return;

					{
						SymlinkListeners::iterator it = m_dirListeners.find(absPath);
						if (it != m_dirListeners.end()) {
							it->second->AddAlias(path);
							m_aliasedListeners[path] = it->second;
							return;
						}
					}
					Reference<DirectoryListener> listener = DirectoryListener::Create(absPath, path, logger);
					if (listener != nullptr) {
						m_dirListeners[absPath] = listener;
						m_aliasedListeners[path] = listener;
						onAdded(listener.operator->());
					}
				}

				template<typename OnErasedCallback, typename OnReinsertedCallback>
				inline void Remove(const Path& path, const OnErasedCallback& onErased, const OnReinsertedCallback& onReinserted) {
					{
						SymlinkListeners::iterator it = m_aliasedListeners.find(path);
						if (it == m_aliasedListeners.end()) return;
						else if (it->second->MainAlias() != path) {
							it->second->RemoveAlias(path);
							return;
						}
					}
					std::vector<Path> subAliases;
					const std::wstring pathString = ((std::wstring)path);
					for (SymlinkListeners::const_iterator it = m_aliasedListeners.begin(); it != m_aliasedListeners.end(); ++it) {
						const std::wstring subPath = it->first;
						if (subPath.length() < pathString.length())
							continue;
						bool isSubstr = true;
						for (size_t i = 0; i < pathString.length(); i++)
							if (pathString[i] != subPath[i]) {
								isSubstr = false;
								break;
							}
						if (isSubstr && pathString.length() < subPath.length())
							isSubstr = (subPath[pathString.length()] == L'/');
						if (isSubstr)
							subAliases.push_back(it->first);
					}
					for (size_t i = 0; i < subAliases.size(); i++) {
						const Path& alias = subAliases[i];
						const Reference<DirectoryListener> listener = [&]() -> Reference<DirectoryListener> {
							SymlinkListeners::iterator it = m_aliasedListeners.find(alias);
							if (it == m_aliasedListeners.end()) return nullptr; // This should never happen...
							Reference<DirectoryListener> l = it->second;
							m_aliasedListeners.erase(it);
							return l;
						}();
						if (listener == nullptr) continue;

						const bool thisIsMainAlias = (listener->MainAlias() == alias);
						if (thisIsMainAlias)
							onErased(listener.operator->());
						listener->RemoveAlias(alias);
						if (listener->HasAlias()) {
							if (thisIsMainAlias)
								onReinserted(listener.operator->());
						}
						else m_dirListeners.erase(listener->AbsolutePath());
					}
				}

				inline void Clear() { 
					m_dirListeners.clear(); 
					m_aliasedListeners.clear();
				}

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
				inline DirChangeWatcher(const Path& directory, Logger* logger);
#endif

			public:
				inline virtual ~DirChangeWatcher();

				inline virtual Event<const FileChangeInfo&>& OnFileChanged()const final override { return m_onFileChanged; }

				inline static Reference<DirChangeWatcher> Open(const Path& directory, OS::Logger* logger);
			};

#ifdef _WIN32
			inline void DirChangeWatcher::Poll() {
				std::vector<Path> removedLinks;
				std::vector<Path> addedLinks;
				auto onEvent = [&](DirectoryListener::FileUpdate&& update) {
					update.observer = this;
					bool isSymlink;
					if (update.changeType == FileChangeType::DELETED)
						isSymlink = m_symlinkListeners.ListeningTo(update.filePath);
					else if (update.changeType == FileChangeType::RENAMED)
						isSymlink = m_symlinkListeners.ListeningTo(update.oldPath.value());
					else {
						std::error_code error;
						isSymlink = std::filesystem::is_symlink(update.filePath, error);
						if (error) isSymlink = false;
					}
					if (isSymlink) {
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

				auto notifyRemoved = [&](DirectoryListener* removed) {
					removed->ForAllFiles([&](const Path& path) {
						FileChangeInfo update;
						update.filePath = path;
						update.changeType = FileChangeType::DELETED;
						update.observer = this;
						QueueEvent(std::move(update));
						});
				};

				auto notifyAdded = [&](DirectoryListener* added) {
					added->ForAllFiles([&](const Path& path) {
						FileChangeInfo update;
						update.filePath = path;
						update.changeType = FileChangeType::CREATED;
						update.observer = this;
						QueueEvent(std::move(update));
						});
				};

				for (size_t i = 0; i < removedLinks.size(); i++)
					m_symlinkListeners.Remove(removedLinks[i], notifyRemoved, notifyAdded);

				for (size_t i = 0; i < addedLinks.size(); i++) {
					const Path& linkPath = addedLinks[i];
					m_symlinkListeners.Add(linkPath, m_rootListener->AbsolutePath(), Log(), notifyAdded);
					Path::IterateDirectory(linkPath, [&](const Path& subpath) {
						std::error_code error;
						bool isSymlink = std::filesystem::is_symlink(subpath, error);
						if ((!error) && isSymlink)
							m_symlinkListeners.Add(subpath, m_rootListener->AbsolutePath(), Log(), notifyAdded);
						return true;
						}, Path::IterateDirectoryFlags::REPORT_DIRECTORIES_RECURSIVE);
				}
			}

			inline DirChangeWatcher::DirChangeWatcher(DirectoryListener* rootListener)
				: DirectoryChangeObserver(rootListener->MainAlias(), rootListener->Log()), m_rootListener(rootListener) {
				Path::IterateDirectory(Directory(), [&](const Path& subpath) {
					std::error_code error;
					bool isSymlink = std::filesystem::is_symlink(subpath, error);
					if ((!error) && isSymlink)
						m_symlinkListeners.Add(subpath, m_rootListener->AbsolutePath(), Log(), [](DirectoryListener*) {});
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
				Path absolutePath;
				try {
					absolutePath = std::filesystem::absolute(directory);
				}
				catch (const std::exception&) {
					logger->Error("DirectoryChangeWatcher::Create - Failed to get absolute path for '", directory, "'!");
					return nullptr; 
				}
				Reference<DirectoryListener> rootDirectoryListener = DirectoryListener::Create(absolutePath, directory, logger);
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

			inline DirChangeWatcher::DirChangeWatcher(const Path& directory, Logger* logger)
				: DirectoryChangeObserver(directory, logger) {
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
					inline Cached(DirectoryChangeObserver* base) : DirectoryChangeObserver(base->Directory(), base->Log()), m_base(base) {}
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

		std::ostream& operator<<(std::ostream& stream, const DirectoryChangeObserver::FileChangeType& type) {
			stream << (
				(type == DirectoryChangeObserver::FileChangeType::NO_OP) ? "NO_OP" :
				(type == DirectoryChangeObserver::FileChangeType::CREATED) ? "CREATED" :
				(type == DirectoryChangeObserver::FileChangeType::DELETED) ? "DELETED" :
				(type == DirectoryChangeObserver::FileChangeType::RENAMED) ? "RENAMED" :
				(type == DirectoryChangeObserver::FileChangeType::CHANGED) ? "CHANGED" :
				(type == DirectoryChangeObserver::FileChangeType::FileChangeType_COUNT) ? "FileChangeType_COUNT" : "<ERROR_TYPE>");
			return stream;
		}

		std::ostream& operator<<(std::ostream& stream, const DirectoryChangeObserver::FileChangeInfo& info) {
			stream << "DirectoryChangeObserver::FileChangeInfo {changeType: " << info.changeType << "; filePath: '" << info.filePath << "'";
			if (info.oldPath.has_value()) stream << "; oldPath: '" << info.oldPath.value() << "'";
			stream << "; observer: " << info.observer << "}";
			return stream;
		}
	}
}

#include "DirectoryChangeObserver.h"
#include "../../Core/Collections/ObjectCache.h"
#include "../../Core/Synch/Semaphore.h"
#include <condition_variable>
#include <thread>
#include <set>
#ifdef _WIN32
#include <windows.h>
#else
#include <map>
#include <chrono>
#include <poll.h>
#include <unistd.h>
#include <sys/inotify.h>
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
				std::unordered_map<Path, std::set<Path>> m_filesPerFolder;

				inline DirectoryListener(const DirectoryListener&) = delete;
				inline DirectoryListener(DirectoryListener&&) = delete;
				inline DirectoryListener& operator=(const DirectoryListener&) = delete;
				inline DirectoryListener& operator=(DirectoryListener&&) = delete;

				template<typename OnRelPathFound>
				inline bool FindAllFiles(const Path& subPath, const OnRelPathFound& onRelPathFound) {
					Path relativePath;
					{
						std::error_code error;
						relativePath = std::filesystem::relative(subPath, m_absolutePath, error);
						if (error) return true;
						if (m_allFilesRelative.find(relativePath) != m_allFilesRelative.end()) return true;
						m_allFilesRelative.insert(relativePath);
						onRelPathFound(std::move(relativePath));
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
					Path::IterateDirectory(subPath, [&](const Path& subPath)->bool {
						std::error_code error;
						const Path relPath = std::filesystem::relative(subPath, m_absolutePath, error);
						if (error) return true;
						m_filesPerFolder[relativePath].insert(relPath);
						return FindAllFiles(subPath, onRelPathFound);
						}, Path::IterateDirectoryFlags::REPORT_ALL);
					return true;
				}

				inline bool AddRelPath(const Path& relativePath) {
					if (m_allFilesRelative.find(relativePath) != m_allFilesRelative.end()) return false;
					m_allFilesRelative.insert(relativePath);
					const Path parentPath = relativePath.parent_path();
					if (parentPath.empty() || parentPath == relativePath) return true;
					m_filesPerFolder[parentPath].insert(relativePath);
					return true;
				}

				inline void RemoveFromFolder(const Path& relativePath) {
					const Path parentPath = relativePath.parent_path();
					if ((!parentPath.empty()) && parentPath != relativePath) {
						decltype(m_filesPerFolder)::iterator it = m_filesPerFolder.find(parentPath);
						if (it != m_filesPerFolder.end()) {
							it->second.erase(relativePath);
							if (it->second.empty())
								m_filesPerFolder.erase(it);
						}
					}
				}

				template<typename EmitChangeCallback>
				inline void RemoveRelPath(const Path& relativePath, const EmitChangeCallback& emitChange) {
					m_allFilesRelative.erase(relativePath);
					RemoveFromFolder(relativePath);
					decltype(m_filesPerFolder)::iterator it = m_filesPerFolder.find(relativePath);
					if (it == m_filesPerFolder.end()) return;
					std::vector<Path> tmp;
					for (std::set<Path>::iterator i = it->second.begin(); i != it->second.end(); ++i)
						tmp.push_back(*i);
					m_filesPerFolder.erase(it);
					for (size_t i = 0; i < tmp.size(); i++) {
						RemoveRelPath(tmp[i], emitChange);
						FileUpdate update;
						update.filePath = MainAlias() / tmp[i];
						update.changeType = DirectoryChangeObserver::FileChangeType::DELETED;
						emitChange(std::move(update));
					}
				}

				template<typename EmitChangeCallback>
				inline void RenameRelPath(const Path& newRelativePath, const Path& oldRelativePath, const EmitChangeCallback& emitChange) {
					AddRelPath(newRelativePath);
					RemoveFromFolder(oldRelativePath);
					m_allFilesRelative.erase(oldRelativePath);
					decltype(m_filesPerFolder)::iterator it = m_filesPerFolder.find(oldRelativePath);
					if (it == m_filesPerFolder.end()) return;
					std::vector<Path> tmp;
					for (std::set<Path>::iterator i = it->second.begin(); i != it->second.end(); ++i)
						tmp.push_back(*i);
					m_filesPerFolder.erase(it);
					for (size_t i = 0; i < tmp.size(); i++) {
						const Path& oldRelPath = tmp[i];
						const Path newRelPath = newRelativePath / tmp[i].filename();
						RenameRelPath(newRelPath, oldRelPath, emitChange);
						FileUpdate update;
						update.filePath = MainAlias() / newRelPath;
						update.oldPath = MainAlias() / oldRelPath;
						update.changeType = DirectoryChangeObserver::FileChangeType::RENAMED;
						emitChange(std::move(update));
					}
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
					Path::IterateDirectory(absPath, [&](const Path& subPath) {
						return FindAllFiles(subPath, [&](Path&&) {});
					}, Path::IterateDirectoryFlags::REPORT_ALL);
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
									if (update.changeType == DirectoryChangeObserver::FileChangeType::NO_OP) return;
									update.filePath = MainAlias() / update.filePath;
									emitResult(std::move(update));
								};
								auto newFileFound = [&]() {
									Path::IterateDirectory(m_absolutePath / update.filePath, [&](const Path& subPath) {
										return FindAllFiles(subPath, [&](Path&& relPath) {
											const Path parentPath = relPath.parent_path();
											if ((!parentPath.empty()) && parentPath != relPath)
												m_filesPerFolder[parentPath].insert(relPath);

											FileUpdate up;
											up.filePath = MainAlias() / relPath;
											up.changeType = DirectoryChangeObserver::FileChangeType::CREATED;
											emitResult(std::move(up));
											});
										}, Path::IterateDirectoryFlags::REPORT_ALL);
								};
								update.filePath = [&]() -> Path {
									std::wstring filePath;
									filePath.resize(notifyInfo->FileNameLength / sizeof(wchar_t));
									const wchar_t* ptr = notifyInfo->FileName;
									for (size_t i = 0; i < filePath.size(); i++)
										filePath[i] = ptr[i];
									return Path(std::move(filePath));
								}();
								if (notifyInfo->Action == FILE_ACTION_RENAMED_OLD_NAME) {
									if (m_fileMovedOldFile.has_value()) {
										FileUpdate& removedFile = m_fileMovedOldFile.value();
										RemoveRelPath(removedFile.filePath, emitResult);
										removedFile.changeType = DirectoryChangeObserver::FileChangeType::DELETED;
										removedFile.filePath = MainAlias() / removedFile.filePath;
										emitResult(std::move(removedFile)); // I guess, it moved somewhere outside our jurisdiction...
									}
									m_fileMovedOldFile = update;
								}
								else if (notifyInfo->Action == FILE_ACTION_RENAMED_NEW_NAME) {
									if (m_fileMovedOldFile.has_value()) {
										RenameRelPath(update.filePath, m_fileMovedOldFile.value().filePath, emitResult);
										update.oldPath = MainAlias() / m_fileMovedOldFile.value().filePath;
										update.changeType = DirectoryChangeObserver::FileChangeType::RENAMED;
										m_fileMovedOldFile = std::optional<FileUpdate>();
										assert(!m_fileMovedOldFile.has_value());
									}
									else {
										if (AddRelPath(update.filePath)) {
											newFileFound();
											update.changeType = DirectoryChangeObserver::FileChangeType::CREATED; // I guess, it moved from somewhere outside our jurisdiction...
										}
										else update.changeType = DirectoryChangeObserver::FileChangeType::NO_OP;
									}
									emitUpdate();
								}
								else {
									update.changeType =
										(notifyInfo->Action == FILE_ACTION_ADDED) ? DirectoryChangeObserver::FileChangeType::CREATED :
										(notifyInfo->Action == FILE_ACTION_REMOVED) ? DirectoryChangeObserver::FileChangeType::DELETED :
										(notifyInfo->Action == FILE_ACTION_MODIFIED) ? DirectoryChangeObserver::FileChangeType::MODIFIED : DirectoryChangeObserver::FileChangeType::NO_OP;
									if (update.changeType != DirectoryChangeObserver::FileChangeType::NO_OP) {
										if (update.changeType == DirectoryChangeObserver::FileChangeType::CREATED) {
											if (AddRelPath(update.filePath)) newFileFound();
											else update.changeType = DirectoryChangeObserver::FileChangeType::NO_OP;
										}
										else if (update.changeType == DirectoryChangeObserver::FileChangeType::DELETED)
											RemoveRelPath(update.filePath, emitResult);
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
						absPath = std::filesystem::canonical(path, error);
						if (error || absPath.empty() || absPath == rootPathAbs) return;
					}

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
			inline static constexpr int NoFd() { return -1; }

			class InotifyInstance;

			class WatchInstance : public virtual ObjectCache<Path>::StoredObject {
			private:
				const Reference<InotifyInstance> m_inotify;
				const Path m_cannonicalPath;
				const int m_watchDescriptor;

				inline WatchInstance(InotifyInstance* inotify, const Path& canPath, int watchDesc) 
					: m_inotify(inotify), m_cannonicalPath(canPath), m_watchDescriptor(watchDesc) {}
				friend class Object;
				friend class InotifyInstance;

			public:
				inline virtual ~WatchInstance();
				inline InotifyInstance* Inotify()const { return m_inotify; }
				inline const Path& CannonicalPath()const { return m_cannonicalPath; }
				inline int WatchDescriptor()const { return m_watchDescriptor; }
			};

			class InotifyInstance : public virtual ObjectCache<Path> {
			private:
				const int m_inotifyFd = NoFd();
				const Reference<Logger> m_logger;

				inline InotifyInstance(int fd, Logger* logger) : m_inotifyFd(fd), m_logger(logger) {}
				friend class Object;
				friend class WatchInstance;
			
			public:
				inline ~InotifyInstance() { close(m_inotifyFd); }

				inline int InotifyFd()const { return m_inotifyFd; }
				inline Logger* Log()const { return m_logger; }

				inline static Reference<InotifyInstance> Create(Logger* logger) {
					const int inotifyFd = inotify_init1(IN_NONBLOCK);
					if (inotifyFd == NoFd()) {
						logger->Error("(DirectoryChangeWatcher::)InotifyInstance::Create - inotify_init() failed! errno= ", (int)errno);
						return nullptr;
					}
					else return Object::Instantiate<InotifyInstance>(inotifyFd, logger);
				}

				inline Reference<WatchInstance> AddWatch(const Path& path) {
					std::error_code error;
					Path canonicalPath = std::filesystem::canonical(path, error);
					if (error) {
						Log()->Error("(DirectoryChangeWatcher::)InotifyInstance::AddWatch - std::filesystem::canonical('", path, "') failed!");
						return nullptr;			
					}
					return GetCachedOrCreate(canonicalPath, [&]()-> Reference<WatchInstance> {	
						const std::string pathText = canonicalPath;
						int watchDescriptor = inotify_add_watch(m_inotifyFd, pathText.c_str(), 
							IN_CREATE | IN_DELETE | IN_MODIFY | IN_CLOSE_WRITE | IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE_SELF);
						if (watchDescriptor == NoFd()) {
							Log()->Error("(DirectoryChangeWatcher::)InotifyInstance::AddWatch - inotify_add_watch('", canonicalPath, "') failed! errno= ", (int)errno);
							return nullptr;
						}
						else return Object::Instantiate<WatchInstance>(this, canonicalPath, watchDescriptor);
					});
				}
			};

			inline WatchInstance::~WatchInstance() {
				std::error_code error;
				bool isValidDir = std::filesystem::is_directory(m_cannonicalPath, error);
				if ((!error) && isValidDir)
					if (inotify_rm_watch(m_inotify->InotifyFd(), m_watchDescriptor) != 0)
						m_inotify->Log()->Error("(DirectoryChangeWatcher::)WatchInstance::~WatchInstance - inotify_rm_watch failed for '", m_cannonicalPath, "'! errno= ", (int)errno);
			}

			class DirectoryListener : public virtual Object {
			private:
				const Path m_directoryPath;
				const Reference<WatchInstance> m_watchInstance;

				inline DirectoryListener(const Path& path, WatchInstance* watch) 
					: m_directoryPath(path), m_watchInstance(watch) {}

				friend class Object; // For Object::Instantiate...
			public:
				inline const Path& Directory()const { return m_directoryPath; }
				inline InotifyInstance* Inotify()const { return m_watchInstance->Inotify(); }
				inline WatchInstance* Watch()const { return m_watchInstance; }

				inline static Reference<DirectoryListener> Open(const Path& path, InotifyInstance* inotify) {
					Reference<WatchInstance> watch = inotify->AddWatch(path);
					if (watch == nullptr) {
						inotify->Log()->Error("(DirectoryChangeWatcher::)DirectoryListener::Open - inotify->AddWatch('", path, "') failed!");
						return nullptr;
					}
					else return Object::Instantiate<DirectoryListener>(path, watch);
				}
			};


#endif



#pragma warning(disable: 4250)
			class DirChangeWatcher : public virtual DirectoryChangeObserver, public virtual ObjectCache<Path>::StoredObject {
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
					static void(*thread)(Callback<>, const std::atomic<bool>*, Semaphore*) = [](Callback<> update, const std::atomic<bool>* dead, Semaphore* firstCycleComplete) {
						update();
						if (firstCycleComplete != nullptr)
							firstCycleComplete->post();
						while (!dead->load()) update();
					};
					Semaphore firstCycleComplete;
					m_pollingThread = std::thread(thread, Callback<>(&DirChangeWatcher::Poll, this), &m_dead, &firstCycleComplete);
					m_notifyThread = std::thread(thread, Callback<>(&DirChangeWatcher::Notify, this), &m_dead, nullptr);
					firstCycleComplete.wait();
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

				Reference<DirectoryListener> m_rootListener;
#ifdef _WIN32
				SymlinkOverlaps m_symlinkListeners;
#else
				alignas(struct inotify_event) char m_readBuffer[1 << 20] = {}; // Excessive? Probably yes, but I don't care...
				size_t m_bytesRead = 0;
				
				class AliasedWatches : public virtual Object {
				private:
					Reference<DirectoryListener> m_mainAlias;
					std::map<Path, Reference<DirectoryListener>> m_aliases;
					std::unordered_set<Path> m_files;

				public:
					inline void AddAlias(DirectoryListener* alias) {
						if (m_aliases.empty()) {
							m_files.clear();
							Path::IterateDirectory(alias->Directory(), [&](const Path& subPath) -> bool { 
								m_files.insert(subPath.filename());
								return true;
							}, Path::IterateDirectoryFlags::REPORT_ALL);
							m_mainAlias = alias;
						}
						m_aliases[alias->Directory()] = alias;
					}
					inline void RemoveAlias(const Path& alias) {
						std::map<Path, Reference<DirectoryListener>>::iterator it = m_aliases.find(alias);
						if (it == m_aliases.end()) return;
						m_aliases.erase(it);
						if (m_aliases.empty())
							m_mainAlias = nullptr;
						else if (m_mainAlias == nullptr || alias == m_mainAlias->Directory())
							m_mainAlias = m_aliases.begin()->second;
					}
					inline DirectoryListener* MainAlias()const { return m_mainAlias; }
					inline bool Empty()const { return m_aliases.empty(); }
					template<typename Callback>
					inline void ForAll(const Callback& callback) {
						for (std::map<Path, Reference<DirectoryListener>>::const_iterator it = m_aliases.begin(); it != m_aliases.end(); ++it)
							callback(it->second.operator->());
					}

					inline void FoundFile(const Path& relPath) { m_files.insert(relPath); }
					inline void RemovedFile(const Path& relPath) { m_files.erase(relPath); }

					template<typename MapFunction>
					inline void ForAllFiles(const MapFunction& callback) {
						for (std::unordered_set<Path>::const_iterator it = m_files.begin(); it != m_files.end(); ++it)
							callback(*it);
					}
				};
				typedef std::unordered_map<int, Reference<AliasedWatches>> WatchDescriptorToAliases;
				WatchDescriptorToAliases m_watchToAliase;
				typedef std::unordered_map<Path, int> AliasesToWatchDescriptor;
				AliasesToWatchDescriptor m_aliasToWatch;

				typedef std::set<int> WatchDescriptorChain;
				template<typename EmitChangeCallback>
				inline void RecordAlias(DirectoryListener* alias, WatchDescriptorChain& chain, const EmitChangeCallback& fileFound) {
					Reference<AliasedWatches> watches = [&]() -> Reference<AliasedWatches> {
						Reference<AliasedWatches>& w = m_watchToAliase[alias->Watch()->WatchDescriptor()];
						if (w == nullptr) w = Object::Instantiate<AliasedWatches>();
						return w;
					}();
					watches->AddAlias(alias);
					m_aliasToWatch[alias->Directory()] = alias->Watch()->WatchDescriptor();
					chain.insert(alias->Watch()->WatchDescriptor());
					watches->ForAll([&](DirectoryListener* al) { 
						watches->ForAllFiles([&](const Path& relPath) {
							const Path file = (al->Directory() / relPath);
							if (watches->MainAlias() == alias) {
								FileChangeInfo changeInfo;
								changeInfo.observer = this;
								changeInfo.filePath = file;
								changeInfo.changeType = FileChangeType::CREATED;
								fileFound(std::move(changeInfo));
							}
							std::error_code error;
							const bool isDirectory = std::filesystem::is_directory(file, error);
							if (error || (!isDirectory)) return;
							Reference<DirectoryListener> listener = DirectoryListener::Open(file, alias->Inotify());
							if (listener == nullptr) return;
							else if (chain.find(listener->Watch()->WatchDescriptor()) != chain.end()) return;
							else RecordAlias(listener, chain, fileFound);
						});
					});
					chain.erase(alias->Watch()->WatchDescriptor());
				}
				template<typename EmitChangeCallback>
				inline void RecordAlias(const Path& file, const EmitChangeCallback& fileFound) {
					std::error_code error;
					const bool isDirectory = std::filesystem::is_directory(file, error);
					if (error || (!isDirectory)) return;
					Reference<DirectoryListener> listener = DirectoryListener::Open(file, m_rootListener->Inotify());
					if (listener == nullptr) return;		
					else if (listener->Watch()->WatchDescriptor() == m_rootListener->Watch()->WatchDescriptor()) return;
					WatchDescriptorChain chain;
					chain.insert(m_rootListener->Watch()->WatchDescriptor());
					RecordAlias(listener, chain, fileFound);
				}
				template<typename EmitChangeCallback>
				inline void RemoveAlias(const Path& file, const EmitChangeCallback& emitChange) {
					AliasesToWatchDescriptor::iterator watchIt = m_aliasToWatch.find(file);
					if (watchIt == m_aliasToWatch.end()) return;
					WatchDescriptorToAliases::iterator it = m_watchToAliase.find(watchIt->second);
					m_aliasToWatch.erase(watchIt);
					if (it == m_watchToAliase.end()) return;
					Reference<AliasedWatches> watches = it->second;
					bool isMainAlias = (watches->MainAlias()->Directory() == file);
					watches->RemoveAlias(file);
					if (watches->Empty())
						m_watchToAliase.erase(it);
					watches->ForAllFiles([&](const Path& relPath) { 
						const Path path = file / relPath;
						RemoveAlias(path, emitChange);
						if (isMainAlias) {
							FileChangeInfo changeInfo;
							changeInfo.observer = this;
							if (watches->Empty()) {
								changeInfo.filePath = path;
								changeInfo.changeType = FileChangeType::DELETED;
							}
							else {
								changeInfo.filePath = watches->MainAlias()->Directory() / relPath;
								changeInfo.oldPath = path;
								changeInfo.changeType = FileChangeType::RENAMED;
							}
							emitChange(std::move(changeInfo));
						}
					});
					watches = nullptr;
				}

				struct PendingRenameRecord : public virtual Object {
					FileChangeInfo info;
					uint32_t cookie = 0;
					const std::chrono::steady_clock::time_point storingTime = std::chrono::steady_clock::now();
				};
				
				typedef std::map<std::chrono::steady_clock::time_point, std::set<Reference<PendingRenameRecord>>> PendingRenamesByTime;
				PendingRenamesByTime m_pendingRenamesByTime;
				typedef std::map<uint32_t, Reference<PendingRenameRecord>> PendingRenamesByCookie;
				PendingRenamesByCookie m_pendingRecordByCookie;
				
				template<typename EmitChangeCallback>
				inline void FlushPendingRenames(const EmitChangeCallback& emitChange) {
					while (m_pendingRenamesByTime.size() > 0) {
						PendingRenamesByTime::iterator it = m_pendingRenamesByTime.begin();
						if (std::chrono::duration<float>(std::chrono::steady_clock::now() - it->first).count() < 1.0f) break;
						for (std::set<Reference<PendingRenameRecord>>::iterator ii = it->second.begin(); ii != it->second.end(); ++ii) {
							Reference<PendingRenameRecord> record = *ii;
							{
								PendingRenamesByCookie::iterator ci = m_pendingRecordByCookie.find(record->cookie);
								if (ci != m_pendingRecordByCookie.end()) 
									if (ci->second == record) 
										m_pendingRecordByCookie.erase(ci);
							}
							FileChangeInfo info = std::move(record->info);
							if (info.oldPath.has_value()) {
								info.changeType = FileChangeType::DELETED;
								info.filePath = info.oldPath.value();
								info.oldPath = std::optional<Path>();
								RemoveAlias(info.filePath, emitChange);
							}
							else {
								info.changeType = FileChangeType::CREATED;
								RecordAlias(info.filePath, emitChange);
							}
							emitChange(std::move(info));
						}
						m_pendingRenamesByTime.erase(it);
					}
				}
				template<typename EmitChangeCallback>
				inline void RecordPendingRename(FileChangeInfo&& info, uint32_t cookie, const EmitChangeCallback& emitChange) {
					auto saveRecord = [&]() {
						Reference<PendingRenameRecord> record = Object::Instantiate<PendingRenameRecord>();
						record->info = std::move(info);
						record->cookie = cookie;
						m_pendingRenamesByTime[record->storingTime].insert(record);
						m_pendingRecordByCookie[cookie] = record;
					};
					PendingRenamesByCookie::iterator it = m_pendingRecordByCookie.find(cookie);
					if (it == m_pendingRecordByCookie.end()) {
						saveRecord();
						return;
					}
					Reference<PendingRenameRecord> record = it->second;
					{
						m_pendingRenamesByTime[record->storingTime].erase(record);
						m_pendingRecordByCookie.erase(it);
					}
					if (record->info.oldPath.has_value() == info.oldPath.has_value()) {
						saveRecord();
						return;
					}
					else if (record->info.oldPath.has_value()) {
						info.oldPath = record->info.oldPath;
					}
					else {
						info.filePath = record->info.filePath;
					}
					RecordAlias(info.filePath, emitChange);
					RemoveAlias(info.oldPath.value(), emitChange);
					emitChange(std::move(info));
				}


				struct ModifiedNoCloseRecord : public virtual Object {
					Path path;
					const std::chrono::steady_clock::time_point storingTime = std::chrono::steady_clock::now();
					inline ModifiedNoCloseRecord(const Path& p) : path(p) {}
				};
				typedef std::map<std::chrono::steady_clock::time_point, std::set<Reference<ModifiedNoCloseRecord>>> ModifiedNoCloseRecordByTime;
				ModifiedNoCloseRecordByTime m_modifiedNoCloseRecordsByTime;
				typedef std::unordered_map<Path, Reference<ModifiedNoCloseRecord>> ModifiedNoCloseRecordPerPath;
				ModifiedNoCloseRecordPerPath m_modifiedNoCloseRecordPerPath;
				inline void ModifiedNoClose(const Path& path) {
					if (m_modifiedNoCloseRecordPerPath.find(path) != m_modifiedNoCloseRecordPerPath.end()) return;
					Reference<ModifiedNoCloseRecord> record = Object::Instantiate<ModifiedNoCloseRecord>(path);
					m_modifiedNoCloseRecordsByTime[record->storingTime].insert(record);
					m_modifiedNoCloseRecordPerPath[record->path] = record;
				}
				inline void ModifiedNoCloseUpdate(const FileChangeInfo& info) {
					auto remove = [&]() {
						ModifiedNoCloseRecordPerPath::iterator recordIt = m_modifiedNoCloseRecordPerPath.find(info.filePath);
						if (recordIt == m_modifiedNoCloseRecordPerPath.end()) return;
						ModifiedNoCloseRecordByTime::iterator timeIt = m_modifiedNoCloseRecordsByTime.find(recordIt->second->storingTime);
						if (timeIt != m_modifiedNoCloseRecordsByTime.end()) {
							timeIt->second.erase(recordIt->second);
							if (timeIt->second.empty())
								m_modifiedNoCloseRecordsByTime.erase(timeIt);
						}
						m_modifiedNoCloseRecordPerPath.erase(recordIt);
					};
					auto rename = [&]() {
						if (!info.oldPath.has_value()) return;
						ModifiedNoCloseRecordPerPath::iterator recordIt = m_modifiedNoCloseRecordPerPath.find(info.oldPath.value());
						if (recordIt == m_modifiedNoCloseRecordPerPath.end()) return;
						Reference<ModifiedNoCloseRecord> record = recordIt->second;
						m_modifiedNoCloseRecordPerPath.erase(recordIt);
						record->path = info.filePath;
						m_modifiedNoCloseRecordPerPath[record->path] = record;
					};
					if (info.changeType == FileChangeType::DELETED || info.changeType == FileChangeType::MODIFIED) remove();
					else if (info.changeType == FileChangeType::RENAMED) rename();
				}
				inline void FlushNoCloseUpdates() {
					while (!m_modifiedNoCloseRecordsByTime.empty()) {
						ModifiedNoCloseRecordByTime::iterator timeIt = m_modifiedNoCloseRecordsByTime.begin();
						if (std::chrono::duration<float>(std::chrono::steady_clock::now() - timeIt->first).count() < 1.0f) break;
						for (std::set<Reference<ModifiedNoCloseRecord>>::iterator it = timeIt->second.begin(); it != timeIt->second.end(); ++it) {
							ModifiedNoCloseRecordPerPath::iterator pathIt = m_modifiedNoCloseRecordPerPath.find(it->operator->()->path);
							if (pathIt == m_modifiedNoCloseRecordPerPath.end()) {
								// Probably an internal error, we should ignore this...
								Log()->Warning("DirectoryChangeObserver::FlushNoCloseUpdates - Inconsistent collections... [File:", __FILE__, "; Line:", __LINE__);
								continue;
							}
							FileChangeInfo info;
							info.filePath = pathIt->first;
							info.changeType = FileChangeType::MODIFIED;
							info.observer = this;
							QueueEvent(std::move(info));
							m_modifiedNoCloseRecordPerPath.erase(pathIt);
						}
						m_modifiedNoCloseRecordsByTime.erase(timeIt);
					}
				}
				
#endif
				inline DirChangeWatcher(DirectoryListener* rootListener);

			public:
				inline virtual ~DirChangeWatcher();

				inline virtual Event<const FileChangeInfo&>& OnFileChanged()const final override { return m_onFileChanged; }

				inline static Reference<DirChangeWatcher> Open(const Path& directory, OS::Logger* logger);
			};
#pragma warning(default: 4250)

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
						else if (update.changeType == FileChangeType::MODIFIED) {
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
				std::error_code error;
				const Path absolutePath = std::filesystem::canonical(directory, error);
				if (error) {
					logger->Error("DirectoryChangeWatcher::Create - Failed to get cannonical path for '", directory, "'!");
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
				auto changeDetected = [&](FileChangeInfo&& changeInfo) {
					ModifiedNoCloseUpdate(changeInfo);
					QueueEvent(std::move(changeInfo));
				};

				// Poll to prevent unnecessary waste of CPU resources:
				const bool canReadBytes = [&]() -> bool {
					struct pollfd pfd = {};
					pfd.fd = m_rootListener->Inotify()->InotifyFd();
					pfd.events = POLLIN;
					int count = poll(&pfd, 1, 1);
					if (count == -1) {
						Log()->Error("DirChangeWatcher::Poll - poll() failed! errno= ", (int)errno);
						return false;
					}
					else if (count <= 0) return false; // We timed out...
					else return true;
				}();

				// Read new data from the file descriptor:
				const bool readNewBytes = [&]() -> bool {
					if (!canReadBytes) return false;
					const size_t availableBytes = sizeof(m_readBuffer) - m_bytesRead;
					ssize_t bytes = read(m_rootListener->Inotify()->InotifyFd(), m_readBuffer + m_bytesRead, availableBytes);
					if (bytes == -1) {
						if (errno != EAGAIN) 
							Log()->Error("DirChangeWatcher::Poll - read() failed! errno=", (int)errno);
						return false;
					}
					else if (bytes <= 0) return false; // Nothing to interpret...
					else if (bytes > sizeof(m_readBuffer)) {
						Log()->Error("DirChangeWatcher::Poll - read() returned size that implies buffer overflow!");
						return false;
					}
					else m_bytesRead += bytes;
					return true;
				}();

				// Process result buffer:
				if (readNewBytes) {
					// Analise inotify_event entries:
					const char* ptr = m_readBuffer;
					const char* end = ptr + m_bytesRead;
					while (ptr < end) {
						const struct inotify_event* event = reinterpret_cast<const struct inotify_event*>(ptr);
						
						// If the buffer is incomplete, we break and hope to complete it on the next iteration
						{
							const size_t bytesLeft = (end - ptr);
							size_t bytesToRead = sizeof(decltype(event->len)) + (reinterpret_cast<const char*>(&event->len) - ptr);
							if (bytesToRead > bytesLeft) break;
							bytesToRead = sizeof(inotify_event) + event->len;
							if (bytesToRead > bytesLeft) {
								if (bytesToRead > sizeof(m_readBuffer)) {
									Log()->Error("DirChangeWatcher::Poll - buffer overflow!");
									ptr = end;
								}
								break;
							} 
							else ptr += bytesToRead;
						}

						if (event->len == 0) continue; // Probably the directory itself... We don't care about these
						else if (event->name[event->len - 1] != '\0') {
							Log()->Error("DirChangeWatcher::Poll - buffer malformed!");
							ptr = end;
							break;
						}

						WatchDescriptorToAliases::iterator aliasIt = m_watchToAliase.find(event->wd);
						if (aliasIt == m_watchToAliase.end()) continue; // We do not care about this one...
						auto fileName = [&]() -> Path { return aliasIt->second->MainAlias()->Directory() / Path(event->name); };

						FileChangeInfo changeInfo;
						changeInfo.observer = this;
						auto hasFlag = [&](decltype(event->mask) mask) { return (event->mask & mask) != 0; };
						if (hasFlag(IN_MOVED_FROM)) {
							changeInfo.oldPath = fileName();
							changeInfo.changeType = FileChangeType::RENAMED;
							aliasIt->second->RemovedFile(event->name);
							RemoveAlias(changeInfo.filePath, changeDetected);
							RecordPendingRename(std::move(changeInfo), event->cookie, changeDetected);
						}
						else if (hasFlag(IN_MOVED_TO)) {
							changeInfo.filePath = fileName();
							changeInfo.changeType = FileChangeType::RENAMED;
							aliasIt->second->FoundFile(event->name);
							RecordAlias(changeInfo.filePath, changeDetected);
							RecordPendingRename(std::move(changeInfo), event->cookie, changeDetected);
						}
						else {
							changeInfo.filePath = fileName();
							changeInfo.changeType = (
								hasFlag(IN_CLOSE_WRITE) ? FileChangeType::MODIFIED :
								hasFlag(IN_CREATE) ? FileChangeType::CREATED : 
								hasFlag(IN_DELETE) ? FileChangeType::DELETED : FileChangeType::NO_OP);
							if (changeInfo.changeType != FileChangeType::NO_OP) {
								if (changeInfo.changeType == FileChangeType::CREATED) {
									aliasIt->second->FoundFile(event->name);
									RecordAlias(changeInfo.filePath, changeDetected);
								}
								else if (changeInfo.changeType == FileChangeType::DELETED) {
									aliasIt->second->RemovedFile(event->name);
									RemoveAlias(changeInfo.filePath, changeDetected);
								}
								changeDetected(std::move(changeInfo));
							}
							else if (hasFlag(IN_MODIFY))
								ModifiedNoClose(changeInfo.filePath);
						}
					}
					
					// Move the unprocessed buffer "down":
					m_bytesRead = (end - ptr);
					ptr = end - m_bytesRead;
					for (size_t i = 0; i < m_bytesRead; i++)
						m_readBuffer[i] = ptr[i];
				}

				FlushPendingRenames(changeDetected);
				FlushNoCloseUpdates();
			}

			inline DirChangeWatcher::DirChangeWatcher(DirectoryListener* rootListener)
				: DirectoryChangeObserver(rootListener->Directory(), rootListener->Inotify()->Log()), m_rootListener(rootListener) {
				WatchDescriptorChain chain;
				RecordAlias(rootListener, chain, [](const FileChangeInfo&) {});
				StartThreads();
			}

			inline DirChangeWatcher::~DirChangeWatcher() {
				KillThreads();
				m_watchToAliase.clear();
				m_aliasToWatch.clear();
				m_rootListener = nullptr;
			}

			inline Reference<DirChangeWatcher> DirChangeWatcher::Open(const Path& directory, OS::Logger* logger) {
				const Reference<InotifyInstance> inotify = InotifyInstance::Create(logger);
				if (inotify == nullptr) return nullptr;
				const Reference<DirectoryListener> rootListener = DirectoryListener::Open(directory, inotify);
				if (rootListener == nullptr) {
					logger->Error("DirectoryChangeWatcher::Create - Failed to add watch for the root directory ('", directory,"')!");
					return nullptr;
				}
				else {
					Reference<DirChangeWatcher> watcher = new DirChangeWatcher(rootListener);
					watcher->ReleaseRef();
					return watcher;
				}
			}
#endif

			class DirChangeWatcherCache : ObjectCache<Path> {
			public:
				static Reference<DirectoryChangeObserver> Open(const Path& directory, OS::Logger* logger) {
					static DirChangeWatcherCache cache;
					return cache.GetCachedOrCreate(directory, [&]()->Reference<DirectoryChangeObserver> {
						return DirChangeWatcher::Open(directory, logger);
						});
				}
			};
		}


		Reference<DirectoryChangeObserver> DirectoryChangeObserver::Create(const Path& directory, OS::Logger* logger, bool cached) {
			assert(logger != nullptr);
			if (cached) return DirChangeWatcherCache::Open(directory, logger);
			else return DirChangeWatcher::Open(directory, logger);
		}

		std::ostream& operator<<(std::ostream& stream, const DirectoryChangeObserver::FileChangeType& type) {
			stream << (
				(type == DirectoryChangeObserver::FileChangeType::NO_OP) ? "NO_OP" :
				(type == DirectoryChangeObserver::FileChangeType::CREATED) ? "CREATED" :
				(type == DirectoryChangeObserver::FileChangeType::DELETED) ? "DELETED" :
				(type == DirectoryChangeObserver::FileChangeType::RENAMED) ? "RENAMED" :
				(type == DirectoryChangeObserver::FileChangeType::MODIFIED) ? "MODIFIED" :
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

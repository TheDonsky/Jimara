#include "MMappedFile.h"
#include "../../Core/Collections/ObjectCache.h"
#include <string>
#ifdef _WIN32
#include <windows.h>
#else
#include "../../Core/Stopwatch.h"
extern "C" {
	int flock(int, int);
	inline static int CallFlock(int fd, int operation) { return flock(fd, operation); }	
}
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#endif 


namespace Jimara {
	namespace OS {
		namespace {
#ifdef _WIN32
			typedef HANDLE FileDescriptor;
			struct FileMapping {
				HANDLE mappingHandle = INVALID_HANDLE_VALUE;
				void* mappedData = nullptr;
				size_t mappedSize = 0u;
			};
#else
			typedef int FileDescriptor;
			struct FileMapping {
				void* mappedData = nullptr;
				size_t mappedSize = 0u;
			};
#endif

			bool OpenFile(const Path& filename, bool writePermission, bool clearFile, FileDescriptor& file, OS::Logger* logger);

			void CloseFile(FileDescriptor desc, OS::Logger* logger);

			bool MapFile(FileDescriptor desc, bool writePermission, bool clearFile, size_t clearedFileSize, FileMapping& mapping, OS::Logger* logger);

			void UnmapFile(const FileMapping& mapping, OS::Logger* logger);


			class MemoryMappedFile : public virtual MMappedFile {
			private:
				const Reference<OS::Logger> m_logger;
				FileDescriptor m_file;
				FileMapping m_mapping;

				MemoryMappedFile(OS::Logger* logger, FileDescriptor file, const FileMapping& mapping) 
					: m_logger(logger), m_file(file), m_mapping(mapping) {}

			public:
				virtual ~MemoryMappedFile() {
					UnmapFile(m_mapping, m_logger);
					CloseFile(m_file, m_logger);
				}

				virtual operator MemoryBlock()const override { return MemoryBlock(m_mapping.mappedData, m_mapping.mappedSize, this); }

				inline static Reference<MMappedFile> Open(const Path& filename, OS::Logger* logger) {
					FileDescriptor file;
					if (!OpenFile(filename, false, false, file, logger)) return nullptr;
					FileMapping mapping;
					if (!MapFile(file, false, false, 0, mapping, logger)) {
						CloseFile(file, logger);
						return nullptr;
					}
					Reference<MemoryMappedFile> instance = new MemoryMappedFile(logger, file, mapping);
					instance->ReleaseRef();
					return instance;
				}
			};



#ifdef _WIN32
			bool OpenFile(const Path& filename, bool writePermission, bool clearFile, FileDescriptor& file, OS::Logger* logger) {
				const std::wstring filePath = filename;
				file = CreateFileW(
					filePath.c_str(),
					GENERIC_READ | (writePermission ? GENERIC_WRITE : 0),
					writePermission ? 0 : FILE_SHARE_READ, NULL,
					writePermission ? (clearFile ? CREATE_ALWAYS : OPEN_ALWAYS) : OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL, NULL);
				if (file == INVALID_HANDLE_VALUE) {
					if (logger != nullptr) logger->Error("MMappedFile::OpenFile - CreateFileA(\"", filename, "\") Failed!");
					return false;
				}
				else return true;
			}

			void CloseFile(FileDescriptor desc, OS::Logger* logger) {
				BOOL rv = (desc == INVALID_HANDLE_VALUE) || CloseHandle(desc);
				if (logger != nullptr && rv == 0)
					logger->Error("MMappedFile::CloseFile - CloseHandle(desc) Failed! <rv=", rv, ">");
			}

			HANDLE CreateFileMappingObject(FileDescriptor desc, bool writePermission, bool clearFile, size_t clearedFileSize, OS::Logger* logger) {
				DWORD sizeH, sizeL;
				if (writePermission && clearFile) {
					sizeH = static_cast<DWORD>(clearedFileSize >> sizeof(DWORD));
					sizeL = static_cast<DWORD>(clearedFileSize);
				}
				else sizeH = sizeL = 0;
				HANDLE handle = CreateFileMappingA(
					desc, NULL,
					writePermission ? PAGE_READWRITE : PAGE_READONLY,
					sizeH, sizeL, NULL);
				if (handle == NULL) {
					if (logger != nullptr) logger->Error("MMappedFile::CreateFileMappingObject - CreateFileMappingA() Failed!");
					return NULL;
				}
				else return handle;
			}

			void DestroyFileMappingHandle(HANDLE handle, OS::Logger* logger) {
				BOOL rv = (handle == INVALID_HANDLE_VALUE) || CloseHandle(handle);
				if (logger != nullptr && rv == 0)
					logger->Error("MMappedFile::DestroyFileMappingHandle - CloseHandle(handle) Failed! <rv=", rv, ">");
			}

			bool MapFile(FileDescriptor desc, bool writePermission, bool clearFile, size_t clearedFileSize, FileMapping& mapping, OS::Logger* logger) {
				if (writePermission && clearFile) mapping.mappedSize = clearedFileSize;
				else {
					LARGE_INTEGER size;
					if (GetFileSizeEx(desc, &size) == 0) {
						if (logger != nullptr) logger->Error("MMappedFile::MapFile - GetFileSize() failed!");
						return false;
					}
					mapping.mappedSize = static_cast<size_t>(min(static_cast<ULONGLONG>(~size_t(0)), static_cast<ULONGLONG>(size.QuadPart)));
				}
				if (mapping.mappedSize <= 0) return true;
				mapping.mappingHandle = CreateFileMappingObject(desc, writePermission, clearFile, clearedFileSize, logger);
				if (mapping.mappingHandle == NULL) return false;
				mapping.mappedData = MapViewOfFileEx(
					mapping.mappingHandle,
					FILE_MAP_READ | (writePermission ? FILE_MAP_WRITE : 0),
					0, 0, 0, NULL);
				if (mapping.mappedData == NULL) {
					if (logger != nullptr) logger->Error("MMappedFile::MapFile - MapViewOfFile() failed!");
					DestroyFileMappingHandle(mapping.mappingHandle, logger);
					return false;
				}
				return true;
			}

			void UnmapFile(const FileMapping& mapping, OS::Logger* logger) {
				if (mapping.mappedData != nullptr)
					if (UnmapViewOfFile(mapping.mappedData) == 0)
						if (logger != nullptr) logger->Error("MMappedFile::UnmapFile - UnmapViewOfFile() Failed!");
				DestroyFileMappingHandle(mapping.mappingHandle, logger);
			}





#else
			bool OpenFile(const Path& filename, bool writePermission, bool clearFile, FileDescriptor& file, OS::Logger* logger) {
				const std::string filePath = filename;
				const int LOCK_FLAG = 
#ifdef O_SHLOCK
					writePermission ? O_EXLOCK : O_SHLOCK;
#else
					0;
#endif
				file = open(
					filePath.c_str(), 
					writePermission ? (O_RDWR | (clearFile ? (O_CREAT | O_TRUNC | LOCK_FLAG) : 0)) : (O_RDONLY | LOCK_FLAG));
				if (file < 0) {
					if (logger != nullptr) logger->Error("MMappedFile::OpenFile - open(\"", filename, "\") Failed!");
					return false;
				}
#ifndef O_SHLOCK
				auto tryFlock = [&]() {
					return CallFlock(file, (writePermission ? LOCK_EX : LOCK_SH) | LOCK_NB); 
				};
				if (tryFlock() == 0) return true;
				else {
					Stopwatch flockTime;
					while (true) {
						if ((errno != EWOULDBLOCK) || (flockTime.Elapsed() > 2.0f)) {
							close(file);
							file = -1;
							if (logger != nullptr) logger->Error("MMappedFile::OpenFile - flock(\"", filename, "\") failed!");
							return false;
						}
						else if (tryFlock() == 0) return true;
					}
				}
#else
				return true;
#endif
			}

			void CloseFile(FileDescriptor desc, OS::Logger* logger) {
				int rv = close(desc);
				if (rv != 0 && logger != nullptr) logger->Error("MMappedFile::CloseFile - close(desc) Failed! <rv=", rv, ">");
			}

			bool MapFile(FileDescriptor desc, bool writePermission, bool clearFile, size_t clearedFileSize, FileMapping& mapping, OS::Logger* logger) {
				if (writePermission && clearFile) mapping.mappedSize = clearedFileSize;
				else {
					mapping.mappedSize = lseek(desc, 0, SEEK_END);
				}
				if (mapping.mappedSize <= 0) return true;
				mapping.mappedData = mmap(
					NULL, mapping.mappedSize, 
					PROT_READ | (writePermission ? PROT_WRITE : 0), MAP_SHARED,
					desc, 0);
				if (mapping.mappedData == MAP_FAILED) {
					if (logger != nullptr) logger->Error("MMappedFile::MapFile - mmap() Failed!");
					return false;
				}
				else return true;
			}

			void UnmapFile(const FileMapping& mapping, OS::Logger* logger) {
				int rv = (mapping.mappedData == nullptr) || munmap(mapping.mappedData, mapping.mappedSize);
				if (rv != 0 && logger != nullptr) logger->Error("MMappedFile::CloseFile - munmap() Failed! <rv=", rv, ">");
			}

#endif


			class MMappedFileCacheStr : ObjectCache<Path> {
			private:
#pragma warning(disable: 4250)
				class Stored : public virtual MMappedFile, public virtual ObjectCache<Path>::StoredObject {
				private:
					const Reference<const MMappedFile> m_back;

				public:
					inline Stored(const MMappedFile* file) : m_back(file) {}

					virtual operator MemoryBlock()const override { 
						MemoryBlock block(*m_back);
						return MemoryBlock(block.Data(), block.Size(), this);
					}
				};
#pragma warning(default: 4250)

			public:
				static Reference<MMappedFile> Open(const Path& filename, OS::Logger* logger) {
					static MMappedFileCacheStr cache;
					return cache.GetCachedOrCreate(filename, [&]()->Reference<MMappedFile> {
						Reference<const MMappedFile> back = MemoryMappedFile::Open(filename, logger);
						if (back == nullptr) return nullptr;
						else return Object::Instantiate<Stored>(back);
						});
				}
			};
		}


		Reference<MMappedFile> MMappedFile::Create(const Path& filename, OS::Logger* logger, bool cached) {
			if (cached) return MMappedFileCacheStr::Open(filename, logger);
			else return MemoryMappedFile::Open(filename, logger);
		}
	}
}

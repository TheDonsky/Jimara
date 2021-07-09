#include "MMappedFile.h"
#include "../../Core/Collections/ObjectCache.h"
#include <string>
#ifdef _WIN32
#include <windows.h>
#else
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

			bool OpenFile(const std::string_view& filename, bool writePermission, bool clearFile, FileDescriptor& file, OS::Logger* logger);

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

				inline static Reference<MMappedFile> Open(const std::string_view& filename, OS::Logger* logger) {
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
			bool OpenFile(const std::string_view& filename, bool writePermission, bool clearFile, FileDescriptor& file, OS::Logger* logger) {
				file = CreateFileA(
					filename.data(),
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
				BOOL rv = CloseHandle(desc);
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
				BOOL rv = CloseHandle(handle);
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

			}

			void UnmapFile(const FileMapping& mapping, OS::Logger* logger) {
				if (UnmapViewOfFile(mapping.mappedData) == 0)
					if (logger != nullptr) logger->Error("MMappedFile::UnmapFile - UnmapViewOfFile() Failed!");
				DestroyFileMappingHandle(mapping.mappingHandle, logger);
			}





#else
			bool OpenFile(const std::string_view& filename, bool writePermission, bool clearFile, FileDescriptor& file, OS::Logger* logger) {
				file = open(
					filename.data(), 
					writePermission ? (O_RDWR | (clearFile ? 0 : 0)) : O_RDONLY);
				if (file < 0) {
					if (logger != nullptr) logger->Error("MMappedFile::OpenFile - open(\"", filename, "\") Failed!");
					return false;
				}
				else return true;
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
				int rv = munmap(mapping.mappedData, mapping.mappedSize);
				if (rv != 0 && logger != nullptr) logger->Error("MMappedFile::CloseFile - munmap() Failed! <rv=", rv, ">");
			}

#endif


			class MMappedFileCacheStr : ObjectCache<std::string> {
			private:
				class Stored : public virtual MMappedFile, public virtual ObjectCache<std::string>::StoredObject {
				private:
					const Reference<const MMappedFile> m_back;

				public:
					inline Stored(const MMappedFile* file) : m_back(file) {}

					virtual operator MemoryBlock()const override { 
						MemoryBlock block(*m_back);
						return MemoryBlock(block.Data(), block.Size(), this);
					}
				};

			public:
				static Reference<MMappedFile> Open(const std::string_view& filename, OS::Logger* logger) {
					static MMappedFileCacheStr cache;
					return cache.GetCachedOrCreate(std::string(filename), false, [&]()->Reference<MMappedFile> {
						Reference<const MMappedFile> back = MemoryMappedFile::Open(filename, logger);
						if (back == nullptr) return nullptr;
						else return Object::Instantiate<Stored>(back);
						});
				}
			};
		}


		Reference<MMappedFile> MMappedFile::Create(const std::string_view& filename, OS::Logger* logger, bool cached) {
			if (cached) return MMappedFileCacheStr::Open(filename, logger);
			else return MemoryMappedFile::Open(filename, logger);
		}
	}
}

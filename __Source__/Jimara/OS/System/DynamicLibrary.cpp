#include "DynamicLibrary.h"
#include "../../Core/Collections/ObjectCache.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include <cassert>


namespace Jimara {
	namespace OS {
#pragma warning(disable: 4250)
		class DynamicLibrary::Implementation : public virtual DynamicLibrary, public virtual ObjectCache<Path>::StoredObject {
		private:
			const Reference<Logger> m_logger;
#ifdef _WIN32
			const HMODULE m_module;

#else 
			void* const m_module;
#endif



		public:
#ifdef _WIN32
			inline Implementation(HMODULE dllModule, Logger* logger) 
				: m_logger(logger), m_module(dllModule) {}
			inline virtual ~Implementation() {
				if (FreeLibrary(m_module) != TRUE)
					if (m_logger != nullptr) m_logger->Error(
						"DynamicLibrary::Implementation(Win32)::~Implementation - FreeLibrary failed",
						" (Error: ", GetLastError(), ")! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			inline static Reference<DynamicLibrary> Create(const Path& path, Logger* logger) {
				std::wstring libPath = path;
				for (size_t i = 0; i < libPath.length(); i++)
					if (libPath[i] == '/') libPath[i] = '\\';
				HMODULE dllModule = LoadLibraryW(libPath.c_str());
				if (dllModule == NULL) {
					if (logger != nullptr)
						logger->Error(
							"DynamicLibrary::Implementation(Win32)::Create - LoadLibraryW failed for '", libPath, "'",
							" (Error: ", GetLastError(), ")! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
				}
				else return Object::Instantiate<Implementation>(dllModule, logger);
			}

			inline void* GetFunctionPtr(const char* name)const {
				FARPROC proc = GetProcAddress(m_module, name);
				if (proc == NULL) {
					if (m_logger != nullptr)
						m_logger->Error(
							"DynamicLibrary::Implementation(Win32)::GetFunctionPtr - GetProcAddress failed for '", name, "'",
							" (Error: ", GetLastError(), ")! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				}
				return (void*)proc;
			}
#else 
			inline Implementation(void* dllModule, Logger* logger)
				: m_logger(logger), m_module(dllModule) {}
			inline virtual ~Implementation() {
				int error = dlclose(m_module);
				if (error != 0)
					if (m_logger != nullptr) m_logger->Error(
						"DynamicLibrary::Implementation(Linux)::~Implementation - dlclose failed",
						" (Error: ", error, ")! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			inline static Reference<DynamicLibrary> Create(const Path& path, Logger* logger) {
				const std::string libPath = path;
				void* const dllModule = dlopen(libPath.c_str(), RTLD_NOW | RTLD_LOCAL);
				if (dllModule == NULL) {
					if (logger != nullptr)
						logger->Error(
							"DynamicLibrary::Implementation(Linux)::Create - dlopen failed for '", libPath, "'",
							" (Error: ", dlerror(), ")! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
				}
				else return Object::Instantiate<Implementation>(dllModule, logger);
			}

			inline void* GetFunctionPtr(const char* name)const {
				void* const symbol = dlsym(m_module, name);
				if (symbol == NULL) {
					if (m_logger != nullptr)
						m_logger->Error(
							"DynamicLibrary::Implementation(Linux)::GetFunctionPtr - dlsym returned nullptr for '", name, "'",
							" (Error: ", dlerror(), ")! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				}
				return symbol;
			}
#endif

			class Cache : public virtual ObjectCache<Path> {
			public:
				inline static Reference<DynamicLibrary> Load(const Path& path, Logger* logger) {
					static Cache cache;
					return cache.GetCachedOrCreate(path, [&]() -> Reference<ObjectCache<Path>::StoredObject> {
						return Create(path, logger);
						});
				}
			};
		};
#pragma warning(default: 4250)

		Reference<DynamicLibrary> DynamicLibrary::Load(Path path, Logger* logger) {
			{
				const std::string extension = Path(path.extension());
				if (extension.length() <= 0u) path += FileExtension();
			}
			std::error_code errorCode;
			const Path absPath = std::filesystem::canonical(path, errorCode);
			if (errorCode) {
				if (logger != nullptr)
					logger->Error(
						"DynamicLibrary::Implementation::Load - Failed to get canonical path of '", path, "'",
						" (Error: ", errorCode.message(), ")! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return nullptr;
			}
			return Implementation::Cache::Load(absPath, logger);
		}

		const std::string_view DynamicLibrary::FileExtension() {
#ifdef _WIN32
			return ".dll";
#else
			return ".so";
#endif
		}

		void* DynamicLibrary::GetFunctionPointer(const char* name)const {
			if (name == nullptr) return nullptr;
			else return dynamic_cast<const Implementation*>(this)->GetFunctionPtr(name);
		}
	}
}

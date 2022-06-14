#include "DynamicLibrary.h"
#include "../../Core/Collections/ObjectCache.h"
#ifdef _WIN32
#include <windows.h>
#else
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
			// __TODO__: Implement this crap!
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
			inline Implementation(Logger* logger)
				: m_logger(logger) {
				// __TODO__: Implement this crap!
			}
			inline virtual ~Implementation() {
				// __TODO__: Implement this crap!
			}

			inline static Reference<DynamicLibrary> Create(const Path& path, Logger* logger) {
				// __TODO__: Implement this crap!
				if (logger != nullptr)
					logger->Error("DynamicLibrary::Implementation(Linux)::Create - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return nullptr;
			}

			inline void* GetFunctionPtr(const char* name)const {
				// __TODO__: Implement this crap!
				if (logger != nullptr)
					logger->Error("DynamicLibrary::Implementation(Linux)::GetFunctionPtr - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return nullptr;
			}
#endif

			class Cache : public virtual ObjectCache<Path> {
			public:
				inline static Reference<DynamicLibrary> Load(const Path& path, Logger* logger) {
					static Cache cache;
					return cache.GetCachedOrCreate(path, false, [&]() -> Reference<ObjectCache<Path>::StoredObject> {
						return Create(path, logger);
						});
				}
			};
		};
#pragma warning(default: 4250)

		Reference<DynamicLibrary> DynamicLibrary::Load(const Path& path, Logger* logger) {
			return Implementation::Cache::Load(path, logger);
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

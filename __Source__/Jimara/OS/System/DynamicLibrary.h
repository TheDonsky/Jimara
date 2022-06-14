#pragma once
#include "../IO/Path.h"
#include "../Logging/Logger.h"
#include <string_view>


namespace Jimara {
	namespace OS {
		/// <summary>
		/// Loaded Dynamic Link Library (DLL[Windows]/SO[Linux]) module
		/// </summary>
		class DynamicLibrary : public virtual Object {
		public:
			/// <summary>
			/// Loads DLL from given path 
			/// <para/> Note that actual module will always be loaded once per file and retrieved from the cache on repeated Load() requests.
			/// </summary>
			/// <param name="path"> Path to the module (.dll/.so) </param>
			/// <param name="logger"> System logger for error reporting </param>
			/// <returns> Dynamic library module </returns>
			static Reference<DynamicLibrary> Load(const Path& path, Logger* logger);

			/// <summary> '.dll' for Win32, '.so' for Linux </summary>
			static const std::string_view FileExtension();

			/// <summary>
			/// Simple type definition for arbitrary function pointer
			/// </summary>
			/// <typeparam name="ReturnType"> Return value type </typeparam>
			/// <typeparam name="...Args"> Passed arguments </typeparam>
			template<typename ReturnType, typename... Args>
			struct FunctionPtr {
				/// <summary> Function pointer type definition </summary>
				typedef ReturnType(*Type)(Args...);
			};

			/// <summary>
			/// Gets function pointer from the library by name
			/// </summary>
			/// <typeparam name="ReturnType"> Return value type </typeparam>
			/// <typeparam name="...Args"> Arguments, passed to the function </typeparam>
			/// <param name="name"> Function name (C-string) </param>
			/// <returns> Function pointer </returns>
			template<typename ReturnType, typename... Args>
			inline typename FunctionPtr<ReturnType, Args...>::Type GetFunction(const char* name)const {
				return (typename FunctionPtr<ReturnType, Args...>::Type)GetFunctionPointer(name);
			}

		private:
			// OS-Specific implementation
			class Implementation;

			// Actual body of GetSymbol()
			void* GetFunctionPointer(const char* name)const;

			// Constructor is private..
			inline DynamicLibrary() {}
		};
	}
}

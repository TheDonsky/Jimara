#ifdef _WIN32
#define TEST_DLL_EXPORT __declspec(dllexport)
#else
#define TEST_DLL_EXPORT
#endif
#include <cstdint>
#include <string_view>

namespace {
	typedef void(*ExecuteFn)(void*);

	struct ExecuteOnDestroy {
		ExecuteFn function = nullptr;
		void* args = nullptr;

		inline ~ExecuteOnDestroy() {
			if (function != nullptr) function(args);
		}

		static ExecuteOnDestroy& Instance() {
			static ExecuteOnDestroy instance;
			return instance;
		}
	};

	static std::string_view initializationState = "NOT INITIALIZED";

	static thread_local int threadLocalValue = -1;

	void DLL_Main() {
		initializationState = "DLL_A INITIALIZED";
	}
}

extern "C" {
	TEST_DLL_EXPORT uint32_t TestDLL_A_Get77773() { return 77773; }
	
	TEST_DLL_EXPORT const char* TestDLL_GetName() { return "DLL_A"; }

#pragma warning(disable: 4190)
	TEST_DLL_EXPORT std::string_view TestDLL_InitializationState() { return initializationState; }
#pragma warning(default: 4190)
	
	TEST_DLL_EXPORT int TestDLL_ThreadLocalCounter() {
		threadLocalValue++;
		return threadLocalValue;
	}
	
	TEST_DLL_EXPORT void TestDLL_ExecuteOnUnload(ExecuteFn fn, void* data) {
		ExecuteOnDestroy& instance = ExecuteOnDestroy::Instance();
		instance.function = fn;
		instance.args = data;
	}
}

#ifdef _WIN32
#include <windows.h>
BOOL WINAPI DllMain(_In_ HINSTANCE, _In_ DWORD fdwReason, _In_ LPVOID) {
	if (fdwReason == DLL_PROCESS_ATTACH) DLL_Main();
	return TRUE;
}
#else
__attribute__((constructor))
static void DllMain() { DLL_Main(); }
#endif

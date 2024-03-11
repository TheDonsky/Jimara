#include "../__Generated__/JIMARA_SAMPLE_GAME_TYPE_REGISTRY.impl.h"
#include <Jimara-StateMachines/Types.h>
#include <Jimara-StateMachines-Editor/Types.h>

namespace Jimara {
	namespace SampleGame {
		static Reference<StateMachines_TypeRegistry> stateMachinesRegistryInstance = nullptr;
		static Reference<StateMachinesEditor_TypeRegistry> stateMachinesEditorRegistryInstance = nullptr;
		static Reference<SampleGame_TypeRegistry> registryInstance = nullptr;

		inline static void Jimara_SampleGame_OnLibraryLoad() {
			stateMachinesRegistryInstance = StateMachines_TypeRegistry::Instance();
			stateMachinesEditorRegistryInstance = StateMachinesEditor_TypeRegistry::Instance();
			registryInstance = SampleGame_TypeRegistry::Instance();
		}

		inline static void Jimara_SampleGame_OnLibraryUnload() {
			registryInstance = nullptr;
			stateMachinesEditorRegistryInstance = nullptr;
			stateMachinesRegistryInstance = nullptr;
		}
	}
}

extern "C" {
#ifdef _WIN32
#include <windows.h>
	BOOL WINAPI DllMain(_In_ HINSTANCE, _In_ DWORD fdwReason, _In_ LPVOID) {
		if (fdwReason == DLL_PROCESS_ATTACH) Jimara::SampleGame::Jimara_SampleGame_OnLibraryLoad();
		else if (fdwReason == DLL_PROCESS_DETACH) Jimara::SampleGame::Jimara_SampleGame_OnLibraryUnload();
		return TRUE;
	}
#else
	__attribute__((constructor)) static void DllMain() {
		Jimara::SampleGame::Jimara_SampleGame_OnLibraryLoad();
	}
	__attribute__((destructor)) static void OnStaticObjectUnload() {
		Jimara::SampleGame::Jimara_SampleGame_OnLibraryUnload();
	}
#endif
}

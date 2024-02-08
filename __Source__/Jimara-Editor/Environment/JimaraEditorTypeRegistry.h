#pragma once
#include <Jimara/Core/TypeRegistration/TypeRegistartion.h>

#pragma warning(disable: 4251)
#ifdef _WIN32
#ifdef JIMARA_EDITOR_DLL_EXPORTS
/// <summary> Classes/Functions marked with this will be exported in Jimara DLL </summary>
#define JIMARA_EDITOR_API __declspec(dllexport)
#else
/// <summary> Classes/Functions marked with this will be exported in Jimara DLL </summary>
#define JIMARA_EDITOR_API __declspec(dllimport)
#endif
#else
/// <summary> Classes/Functions marked with this will be exported in Jimara DLL </summary>
#define JIMARA_EDITOR_API
#endif

namespace Jimara {
	namespace Editor {
		/// <summary> Type registry for Jimara-Editor </summary>
		JIMARA_DEFINE_TYPE_REGISTRATION_CLASS(JimaraEditorTypeRegistry, JIMARA_EDITOR_API);
	}
}

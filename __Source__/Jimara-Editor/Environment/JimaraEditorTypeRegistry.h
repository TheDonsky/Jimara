#pragma once
#include <Data/TypeRegistration/TypeRegistartion.h>

namespace Jimara {
	namespace Editor {
#define JimaraEditorTypeRegistry_TMP_DLL_EXPORT_MACRO
		/// <summary> Type registry for Jimara-Editor </summary>
		JIMARA_DEFINE_TYPE_REGISTRATION_CLASS(JimaraEditorTypeRegistry, JimaraEditorTypeRegistry_TMP_DLL_EXPORT_MACRO);
#undef JimaraEditorTypeRegistry_TMP_DLL_EXPORT_MACRO
	}
}

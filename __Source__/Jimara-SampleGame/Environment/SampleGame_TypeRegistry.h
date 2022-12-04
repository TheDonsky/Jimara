#pragma once
#include <Jimara/Core/TypeRegistration/TypeRegistartion.h>

namespace Jimara {
	namespace SampleGame {
		JIMARA_REGISTER_TYPE(Jimara::SampleGame::SampleGame_TypeRegistry);
#define JimaraEditorTypeRegistry_TMP_DLL_EXPORT_MACRO
		/// <summary> Type registry for Jimara-SampleGame </summary>
		JIMARA_DEFINE_TYPE_REGISTRATION_CLASS(SampleGame_TypeRegistry, JimaraEditorTypeRegistry_TMP_DLL_EXPORT_MACRO);
#undef JimaraEditorTypeRegistry_TMP_DLL_EXPORT_MACRO
	}
}

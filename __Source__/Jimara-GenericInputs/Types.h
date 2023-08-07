#pragma once
#include <Jimara/Core/TypeRegistration/TypeRegistartion.h>


namespace Jimara {
	JIMARA_REGISTER_TYPE(Jimara::GenericInputs_TypeRegistry);
#define JimaraGenericInputsTypeRegistry_TMP_DLL_EXPORT_MACRO
	/// <summary> Type registry for Jimara-GenericInputs </summary>
	JIMARA_DEFINE_TYPE_REGISTRATION_CLASS(GenericInputs_TypeRegistry, JimaraGenericInputsTypeRegistry_TMP_DLL_EXPORT_MACRO);
#undef JimaraGenericInputsTypeRegistry_TMP_DLL_EXPORT_MACRO
}

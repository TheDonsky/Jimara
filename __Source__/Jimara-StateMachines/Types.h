#pragma once
#include <Jimara/Core/TypeRegistration/TypeRegistartion.h>


namespace Jimara {
	JIMARA_REGISTER_TYPE(Jimara::StateMachines_TypeRegistry);
#define JimaraStateMachinesTypeRegistry_TMP_DLL_EXPORT_MACRO
	/// <summary> Type registry for Jimara-StateMachines </summary>
	JIMARA_DEFINE_TYPE_REGISTRATION_CLASS(StateMachines_TypeRegistry, JimaraStateMachinesTypeRegistry_TMP_DLL_EXPORT_MACRO);
#undef JimaraStateMachinesTypeRegistry_TMP_DLL_EXPORT_MACRO
}

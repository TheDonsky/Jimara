#pragma once
#include <Jimara/Core/TypeRegistration/TypeRegistration.h>


namespace Jimara {
	JIMARA_REGISTER_TYPE(Jimara::StateMachinesEditor_TypeRegistry);
#define JIMARA_STATE_MACHINES_EDITOR_API
	/// <summary> Type registry for Jimara-StateMachines </summary>
	JIMARA_DEFINE_TYPE_REGISTRATION_CLASS(StateMachinesEditor_TypeRegistry, JIMARA_STATE_MACHINES_EDITOR_API);
}

#pragma once
#include <Jimara/Core/TypeRegistration/TypeRegistration.h>


namespace Jimara {
	JIMARA_REGISTER_TYPE(Jimara::StateMachines_TypeRegistry);
#define JIMARA_STATE_MACHINES_API
	/// <summary> Type registry for Jimara-StateMachines </summary>
	JIMARA_DEFINE_TYPE_REGISTRATION_CLASS(StateMachines_TypeRegistry, JIMARA_STATE_MACHINES_API);
}

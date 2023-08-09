#pragma once
#include <Jimara/Core/TypeRegistration/TypeRegistartion.h>


namespace Jimara {
	JIMARA_REGISTER_TYPE(Jimara::GenericInputs_TypeRegistry);
#define JIMARA_GENERIC_INPUTS_API
	/// <summary> Type registry for Jimara-GenericInputs </summary>
	JIMARA_DEFINE_TYPE_REGISTRATION_CLASS(GenericInputs_TypeRegistry, JIMARA_GENERIC_INPUTS_API);
}

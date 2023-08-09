#pragma once
#include <Jimara/Core/TypeRegistration/TypeRegistartion.h>

namespace Jimara {
	namespace SampleGame {
		JIMARA_REGISTER_TYPE(Jimara::SampleGame::SampleGame_TypeRegistry);
#define JIMARA_GENERIC_INPUTS_API
		/// <summary> Type registry for Jimara-SampleGame </summary>
		JIMARA_DEFINE_TYPE_REGISTRATION_CLASS(SampleGame_TypeRegistry, JIMARA_GENERIC_INPUTS_API);
	}
}

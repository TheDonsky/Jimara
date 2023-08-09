#include "KeyCodeInput.h"
#include <Jimara/Data/Serialization/Attributes/EnumAttribute.h>
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>


namespace Jimara {
	namespace OS {
		KeyCodeInput::KeyCodeInput(Component* parent, const std::string_view& name)
			: Component(parent, name) {}

		KeyCodeInput::~KeyCodeInput() {}

		void KeyCodeInput::GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement) {
			Jimara::Component::GetFields(recordElement);
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				JIMARA_SERIALIZE_FIELD_GET_SET(KeyCode, SetKeyCode, "Key", "Key Code", Jimara::OS::Input::KeyCodeEnumAttribute());
				JIMARA_SERIALIZE_FIELD_GET_SET(DeviceId, SetDeviceId, "Device", "Device Id (for gamepads, mostly)");
				JIMARA_SERIALIZE_FIELD_GET_SET(InputMode, SetInputMode, "Mode", "Input mode",
					Object::Instantiate<Jimara::Serialization::EnumAttribute<std::underlying_type_t<Mode>>>(false,
					"NO_INPUT", Mode::NO_INPUT,
					"ON_KEY_DOWN", Mode::ON_KEY_DOWN,
					"ON_KEY_PRESSED", Mode::ON_KEY_PRESSED,
					"ON_KEY_UP", Mode::ON_KEY_UP));
				JIMARA_SERIALIZE_FIELD_GET_SET(InputFlags, SetFlags, "Flags", "Additional input flags/settings", 
					Object::Instantiate<Jimara::Serialization::EnumAttribute<std::underlying_type_t<Flags>>>(true,
					"INVERT_INPUT_MODE", Flags::INVERT_INPUT_MODE,
					"NO_VALUE_ON_FALSE_INPUT", Flags::NO_VALUE_ON_FALSE_INPUT,
					"NO_VALUE_IF_DISABLED", Flags::NO_VALUE_IF_DISABLED));
			};
		}

		std::optional<bool> KeyCodeInput::EvaluateInput() {
			auto hasFlag = [&](Flags flag) { 
				return (static_cast<std::underlying_type_t<Flags>>(m_flags) & static_cast<std::underlying_type_t<Flags>>(flag)) != 0; 
			};
			if (hasFlag(Flags::NO_VALUE_IF_DISABLED) && (!ActiveInHeirarchy()))
				return std::optional<bool>();
			const bool pulse =
				(m_mode == Mode::ON_KEY_DOWN) ? Context()->Input()->KeyDown(m_key, uint8_t(m_deviceId)) :
				(m_mode == Mode::ON_KEY_PRESSED) ? Context()->Input()->KeyDown(m_key, uint8_t(m_deviceId)) :
				(m_mode == Mode::ON_KEY_UP) ? Context()->Input()->KeyDown(m_key, uint8_t(m_deviceId)) : false;
			const bool value = hasFlag(Flags::INVERT_INPUT_MODE) ^ pulse;
			if (hasFlag(Flags::NO_VALUE_ON_FALSE_INPUT) && (!value))
				return std::optional<bool>();
			else return value;
		}
	}
	
	template<> void TypeIdDetails::GetTypeAttributesOf<OS::KeyCodeInput>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<OS::KeyCodeInput> serializer("Jimara/Input/OS/KeyCodeInput", "KeyCodeInput");
		report(&serializer);
	}
}

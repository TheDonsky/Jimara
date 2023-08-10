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
				JIMARA_SERIALIZE_FIELD_GET_SET(Flags, SetFlags, "Flags", "Additional input flags/settings", 
					Object::Instantiate<Jimara::Serialization::EnumAttribute<std::underlying_type_t<InputFlags>>>(true,
					"INVERT_INPUT_MODE", InputFlags::INVERT_INPUT_MODE,
					"NO_VALUE_ON_FALSE_INPUT", InputFlags::NO_VALUE_ON_FALSE_INPUT,
					"NO_VALUE_IF_DISABLED", InputFlags::NO_VALUE_IF_DISABLED));
			};
		}

		std::optional<bool> KeyCodeInput::EvaluateInput() {
			auto hasFlag = [&](InputFlags flag) {
				return (static_cast<std::underlying_type_t<InputFlags>>(m_flags) & static_cast<std::underlying_type_t<InputFlags>>(flag)) != 0;
			};
			if (hasFlag(InputFlags::NO_VALUE_IF_DISABLED) && (!ActiveInHeirarchy()))
				return std::optional<bool>();
			const bool pulse =
				(m_mode == Mode::ON_KEY_DOWN) ? Context()->Input()->KeyDown(m_key, uint8_t(m_deviceId)) :
				(m_mode == Mode::ON_KEY_PRESSED) ? Context()->Input()->KeyDown(m_key, uint8_t(m_deviceId)) :
				(m_mode == Mode::ON_KEY_UP) ? Context()->Input()->KeyDown(m_key, uint8_t(m_deviceId)) : false;
			const bool value = hasFlag(InputFlags::INVERT_INPUT_MODE) ^ pulse;
			if (hasFlag(InputFlags::NO_VALUE_ON_FALSE_INPUT) && (!value))
				return std::optional<bool>();
			else return value;
		}
	}
	
	template<> void TypeIdDetails::GetTypeAttributesOf<OS::KeyCodeInput>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<OS::KeyCodeInput> serializer("Jimara/Input/OS/KeyCodeInput", "KeyCodeInput");
		report(&serializer);
	}
}

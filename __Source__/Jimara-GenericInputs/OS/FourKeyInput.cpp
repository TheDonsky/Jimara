#include "FourKeyInput.h"
#include <Jimara/Data/Serialization/Attributes/EnumAttribute.h>
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>


namespace Jimara {
	namespace OS {
		FourKeyInput::FourKeyInput(Component* parent, const std::string_view& name) 
			: Component(parent, name) {}

		FourKeyInput::~FourKeyInput() {}

		void FourKeyInput::GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement) {
			Jimara::Component::GetFields(recordElement);
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				JIMARA_SERIALIZE_FIELD(m_left, "Left", "Left direction", Jimara::OS::Input::KeyCodeEnumAttribute());
				JIMARA_SERIALIZE_FIELD(m_right, "Right", "Right direction", Jimara::OS::Input::KeyCodeEnumAttribute());
				JIMARA_SERIALIZE_FIELD(m_up, "Up", "Up direction", Jimara::OS::Input::KeyCodeEnumAttribute());
				JIMARA_SERIALIZE_FIELD(m_down, "Down", "Down direction", Jimara::OS::Input::KeyCodeEnumAttribute());
				JIMARA_SERIALIZE_FIELD_GET_SET(DeviceId, SetDeviceId, "Device", "Device Id (for gamepads, mostly)");
				JIMARA_SERIALIZE_FIELD_GET_SET(Flags, SetFlags, "Flags", "Additional input flags/settings",
					Object::Instantiate<Jimara::Serialization::EnumAttribute<std::underlying_type_t<InputFlags>>>(true,
						"NORMALIZE", InputFlags::NORMALIZE,
						"NO_VALUE_ON_NO_INPUT", InputFlags::NO_VALUE_ON_NO_INPUT,
						"NO_VALUE_IF_DISABLED", InputFlags::NO_VALUE_IF_DISABLED));
			};
		}

		std::optional<Vector2> FourKeyInput::EvaluateInput() {
			auto hasFlag = [&](InputFlags flag) {
				return (static_cast<std::underlying_type_t<InputFlags>>(m_flags) & static_cast<std::underlying_type_t<InputFlags>>(flag)) != 0;
			};
			if (hasFlag(InputFlags::NO_VALUE_IF_DISABLED) && (!ActiveInHeirarchy()))
				return std::optional<Vector2>();
			auto input = [&](Jimara::OS::Input::KeyCode code) {
				return Context()->Input()->KeyPressed(code, m_deviceId) ? 1.0f : 0.0f;
			};
			const Vector2 rawInput(
				input(m_right) - input(m_left),
				input(m_up) - input(m_down));
			const float sqrMagn = Math::SqrMagnitude(rawInput);
			if (sqrMagn < std::numeric_limits<float>::epsilon()) {
				if (hasFlag(InputFlags::NO_VALUE_ON_NO_INPUT))
					return std::optional<Vector2>();
				else return Vector2(0.0f);
			}
			else return hasFlag(InputFlags::NORMALIZE) ? (rawInput / std::sqrt(sqrMagn)) : rawInput;
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<OS::FourKeyInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<OS::FourKeyInput>(
			"Four Key Input", "Jimara/Input/OS/FourKeyInput", "Generic 2d input from 2 directional keys");
		report(factory);
	}
}

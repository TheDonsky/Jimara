#include "AxisInput.h"
#include <Jimara/Data/Serialization/Attributes/EnumAttribute.h>
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>


namespace Jimara {
	namespace OS {
		AxisInput::AxisInput(Component* parent, const std::string_view& name)
			: Component(parent, name) {}

		AxisInput::~AxisInput() {}

		void AxisInput::GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement) {
			Jimara::Component::GetFields(recordElement);
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				JIMARA_SERIALIZE_FIELD(m_axis, "Axis", "Axis", Jimara::OS::Input::AxisEnumAttribute());
				JIMARA_SERIALIZE_FIELD_GET_SET(DeviceId, SetDeviceId, "Device", "Device Id (for gamepads, mostly)");
				JIMARA_SERIALIZE_FIELD_GET_SET(Flags, SetFlags, "Flags", "Additional input flags/settings",
					Object::Instantiate<Jimara::Serialization::EnumAttribute<std::underlying_type_t<InputFlags>>>(true,
						"NO_VALUE_ON_NO_INPUT", InputFlags::NO_VALUE_ON_NO_INPUT,
						"NO_VALUE_IF_DISABLED", InputFlags::NO_VALUE_IF_DISABLED));
			};
		}

		std::optional<float> AxisInput::EvaluateInput() {
			auto hasFlag = [&](InputFlags flag) {
				return (static_cast<std::underlying_type_t<InputFlags>>(m_flags) & static_cast<std::underlying_type_t<InputFlags>>(flag)) != 0;
			};
			if (hasFlag(InputFlags::NO_VALUE_IF_DISABLED) && (!ActiveInHeirarchy()))
				return std::optional<float>();
			auto input = [&](Jimara::OS::Input::KeyCode code) {
				return Context()->Input()->KeyPressed(code) ? 1.0f : 0.0f;
			};
			const float value = Context()->Input()->GetAxis(m_axis, m_deviceId);
			if (hasFlag(InputFlags::NO_VALUE_ON_NO_INPUT) && std::abs(value) < std::numeric_limits<float>::epsilon())
				return std::optional<float>();
			else return value;
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<OS::AxisInput>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<OS::AxisInput> serializer("Jimara/Input/OS/AxisInput", "AxisInput");
		report(&serializer);
	}
}

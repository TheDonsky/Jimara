#include "DoubleAxisInput.h"
#include <Jimara/Data/Serialization/Attributes/EnumAttribute.h>
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>


namespace Jimara {
	namespace OS {
		DoubleAxisInput::DoubleAxisInput(Component* parent, const std::string_view& name) 
			: Component(parent, name) {}

		DoubleAxisInput::~DoubleAxisInput() {}

		void DoubleAxisInput::GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement) {
			Jimara::Component::GetFields(recordElement);
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				JIMARA_SERIALIZE_FIELD(m_horizontal, "Horizontal", "Horizontal(X) axis", Jimara::OS::Input::AxisEnumAttribute());
				JIMARA_SERIALIZE_FIELD(m_vertical, "Vertical", "Vertical(Y) axis", Jimara::OS::Input::AxisEnumAttribute());
				JIMARA_SERIALIZE_FIELD_GET_SET(DeviceId, SetDeviceId, "Device", "Device Id (for gamepads, mostly)");
				JIMARA_SERIALIZE_FIELD_GET_SET(Flags, SetFlags, "Flags", "Additional input flags/settings",
					Object::Instantiate<Jimara::Serialization::EnumAttribute<std::underlying_type_t<InputFlags>>>(true,
						"NORMALIZE", InputFlags::NORMALIZE,
						"NO_VALUE_ON_NO_INPUT", InputFlags::NO_VALUE_ON_NO_INPUT,
						"NO_VALUE_IF_DISABLED", InputFlags::NO_VALUE_IF_DISABLED));
				JIMARA_SERIALIZE_FIELD(m_maxMagnitude, "Max magnitude",
					"Maximal magnitude of the output; If NORMALIZE flag is set and MaxMagnitude is not infinite, it'll act as a value scaler ");
				m_maxMagnitude = Math::Max(m_maxMagnitude, 0.0f);
			};
		}

		std::optional<Vector2> DoubleAxisInput::EvaluateInput() {
			auto hasFlag = [&](InputFlags flag) {
				return (static_cast<std::underlying_type_t<InputFlags>>(m_flags) & static_cast<std::underlying_type_t<InputFlags>>(flag)) != 0;
			};
			if (hasFlag(InputFlags::NO_VALUE_IF_DISABLED) && (!ActiveInHeirarchy()))
				return std::optional<Vector2>();
			auto input = [&](Jimara::OS::Input::KeyCode code) {
				return Context()->Input()->KeyPressed(code) ? 1.0f : 0.0f;
			};
			const Vector2 rawInput(
				Context()->Input()->GetAxis(m_horizontal, m_deviceId),
				Context()->Input()->GetAxis(m_vertical, m_deviceId));
			const float sqrMagn = Math::SqrMagnitude(rawInput);
			if (sqrMagn < std::numeric_limits<float>::epsilon()) {
				if (hasFlag(InputFlags::NO_VALUE_ON_NO_INPUT))
					return std::optional<Vector2>();
				else return Vector2(0.0f);
			}
			else if (hasFlag(InputFlags::NORMALIZE)) {
				const Vector2 direction = (rawInput / std::sqrt(sqrMagn));
				return std::isinf(m_maxMagnitude) ? direction : (direction * m_maxMagnitude);
			}
			else {
				const float magn = std::sqrt(sqrMagn);
				if (magn <= m_maxMagnitude)
					return rawInput;
				else return rawInput * (m_maxMagnitude / magn);
			}
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<OS::DoubleAxisInput>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<OS::DoubleAxisInput> serializer("Jimara/Input/OS/DoubleAxisInput", "DoubleAxisInput");
		report(&serializer);
	}
}

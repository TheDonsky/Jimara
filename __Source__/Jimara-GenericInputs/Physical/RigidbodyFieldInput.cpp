#include "RigidbodyFieldInput.h"
#include <Jimara/Data/Serialization/Attributes/EnumAttribute.h>
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>


namespace Jimara {
	RigidbodyFieldInput::RigidbodyFieldInput(Component* parent, const std::string_view& name)
		: Component(parent, name) {
	}

	RigidbodyFieldInput::~RigidbodyFieldInput() {}

	void RigidbodyFieldInput::GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement) {
		Jimara::Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Source, SetSource, "Source",
				"Source component for retrieving fields from\n"
				"If nullptr, as long as FIND_SOURCE_ON_PARENT_CHAIN_IF_NOT_SET flag is set, value will come from a component in parent chain.");
			JIMARA_SERIALIZE_FIELD_GET_SET(Mode, SetMode, "InputMode", "Input InputMode",
				Object::Instantiate<Jimara::Serialization::EnumAttribute<std::underlying_type_t<InputMode>>>(false,
					"VELOCITY", InputMode::VELOCITY,
					"ANGULAR_VELOCITY", InputMode::ANGULAR_VELOCITY,
					"MASS", InputMode::MASS,
					"CCD_ENABLED", InputMode::CCD_ENABLED,
					"GRAVITY", InputMode::GRAVITY,
					"NO_INPUT", InputMode::NO_INPUT));
			JIMARA_SERIALIZE_FIELD_GET_SET(Flags, SetFlags, "No Input If Disabled", "Blocks input if component is disabled",
				Object::Instantiate<Jimara::Serialization::EnumAttribute<std::underlying_type_t<InputFlags>>>(true,
					"NO_VALUE_IF_DISABLED", InputFlags::NO_VALUE_IF_DISABLED));
		};
	}

	std::optional<Vector3> RigidbodyFieldInput::EvaluateInput() {
		auto hasFlag = [&](InputFlags flag) {
			return (static_cast<std::underlying_type_t<InputFlags>>(m_flags) & static_cast<std::underlying_type_t<InputFlags>>(flag)) != 0;
			};
		Jimara::Rigidbody* source = Source();
		if (source == nullptr) 
			source = GetComponentInParents<Jimara::Rigidbody>();
		if (m_mode >= InputMode::NO_INPUT || 
			(source == nullptr && m_mode != InputMode::GRAVITY) || 
			(hasFlag(InputFlags::NO_VALUE_IF_DISABLED) && (!ActiveInHierarchy())))
			return std::optional<Vector3>();
		switch (m_mode) {
		case InputMode::VELOCITY: return source->Velocity();
		case InputMode::ANGULAR_VELOCITY: return source->AngularVelocity();
		case InputMode::MASS: return Vector3(source->Mass(), 0.0f, 0.0f);
		case InputMode::CCD_ENABLED: return Vector3(source->CCDEnabled() ? 1.0f : 0.0f, 0.0f, 0.0f);
		case InputMode::GRAVITY: return (source == nullptr ? Context() : source->Context())->Physics()->Gravity();
		}
		return std::optional<Vector3>();
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<RigidbodyFieldInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<RigidbodyFieldInput>(
			"Rigidbody Field Input", "Jimara/Input/Physical/RigidbodyFieldInput", "Input from Rigidbody component fields");
		report(factory);
	}
}

#include "TransformFieldInput.h"
#include <Jimara/Data/Serialization/Attributes/EnumAttribute.h>
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>


namespace Jimara {
	TransformFieldInput::TransformFieldInput(Component* parent, const std::string_view& name)
		: Component(parent, name) {}

	TransformFieldInput::~TransformFieldInput() {}

	void TransformFieldInput::GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement) {
		Jimara::Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Source, SetSource, "Source",
				"Source transform component\n"
				"If this is missing, there will be an attempt to find it in the parent chain.");
			JIMARA_SERIALIZE_FIELD_GET_SET(Mode, SetMode, "InputMode", "Input InputMode",
				Object::Instantiate<Jimara::Serialization::EnumAttribute<std::underlying_type_t<InputMode>>>(false,
					"WORLD_POSITION", InputMode::WORLD_POSITION,
					"LOCAL_POSITION", InputMode::LOCAL_POSITION,
					"WORLD_ROTATION", InputMode::WORLD_ROTATION,
					"LOCAL_ROTATION", InputMode::LOCAL_ROTATION,
					"WORLD_SCALE", InputMode::WORLD_SCALE,
					"LOCAL_SCALE", InputMode::LOCAL_SCALE,
					"FORWARD", InputMode::FORWARD,
					"LOCAL_FORWARD", InputMode::LOCAL_FORWARD,
					"RIGHT", InputMode::RIGHT,
					"LOCAL_RIGHT", InputMode::LOCAL_RIGHT,
					"UP", InputMode::UP,
					"LOCAL_UP", InputMode::LOCAL_UP,
					"NO_INPUT", InputMode::NO_INPUT));
			JIMARA_SERIALIZE_FIELD_GET_SET(Flags, SetFlags, "No Input If Disabled", "Blocks input if component is disabled",
				Object::Instantiate<Jimara::Serialization::EnumAttribute<std::underlying_type_t<InputFlags>>>(true,
					"NO_VALUE_IF_DISABLED", InputFlags::NO_VALUE_IF_DISABLED,
					"DO_NOT_SEARCH_FOR_SOURCE_TRANSFORM_IN_HIERARCHY", InputFlags::DO_NOT_SEARCH_FOR_SOURCE_TRANSFORM_IN_HIERARCHY));
		};
	}

	std::optional<Vector3> TransformFieldInput::EvaluateInput() {
		auto hasFlag = [&](InputFlags flag) {
			return (static_cast<std::underlying_type_t<InputFlags>>(m_flags) & static_cast<std::underlying_type_t<InputFlags>>(flag)) != 0;
		};
		Jimara::Transform* transform = Source();
		if (transform == nullptr && ((Flags() & InputFlags::DO_NOT_SEARCH_FOR_SOURCE_TRANSFORM_IN_HIERARCHY) == InputFlags::NONE))
			transform = GetTransfrom();
		if (m_mode >= InputMode::NO_INPUT || (transform == nullptr) || (hasFlag(InputFlags::NO_VALUE_IF_DISABLED) && (!ActiveInHierarchy())))
			return std::optional<Vector3>();
		switch (m_mode) {
		case InputMode::WORLD_POSITION: return transform->WorldPosition();
		case InputMode::LOCAL_POSITION: return transform->LocalPosition();
		case InputMode::WORLD_ROTATION: return transform->WorldEulerAngles();
		case InputMode::LOCAL_ROTATION: return transform->LocalEulerAngles();
		case InputMode::WORLD_SCALE: return transform->LossyScale();
		case InputMode::LOCAL_SCALE: return transform->LocalScale();
		case InputMode::FORWARD: return transform->Forward();
		case InputMode::LOCAL_FORWARD: return transform->LocalForward();
		case InputMode::RIGHT: return transform->Right();
		case InputMode::LOCAL_RIGHT: return transform->LocalRight();
		case InputMode::UP: return transform->Up();
		case InputMode::LOCAL_UP: return transform->LocalUp();
		}
		return std::optional<Vector3>();
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<TransformFieldInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<TransformFieldInput>(
			"Transform Field Input", "Jimara/Input/Physical/TransformFieldInput", "Input from Transform component fields");
		report(factory);
	}
}

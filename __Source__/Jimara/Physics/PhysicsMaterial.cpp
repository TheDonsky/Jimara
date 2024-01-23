#include "PhysicsMaterial.h"
#include "../Data/Serialization/Helpers/SerializerMacros.h"
#include "../Data/Serialization/Attributes/EnumAttribute.h"


namespace Jimara {
	namespace Physics {
		const Object* PhysicsMaterial::CombineModeEnumAttribute() {
			static const Reference<const Object> attribute = Object::Instantiate<Serialization::EnumAttribute<std::underlying_type_t<CombineMode>>>(false,
				"AVERAGE", CombineMode::AVERAGE,
				"MIN", CombineMode::MIN,
				"MULTIPLY", CombineMode::MULTIPLY,
				"MAX", CombineMode::MAX);
			return attribute;
		}

		void PhysicsMaterial::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				JIMARA_SERIALIZE_FIELD_GET_SET(StaticFriction, SetStaticFriction, "Static Friction", "Static friction ceofficient");
				JIMARA_SERIALIZE_FIELD_GET_SET(DynamicFriction, SetDynamicFriction, "Dynamic Friction", "Dynamic friction ceofficient");
				JIMARA_SERIALIZE_FIELD_GET_SET(FrictionCombineMode, SetFrictionCombineMode, 
					"Friction Combine", "Combine mode for the friction settings", CombineModeEnumAttribute());
				JIMARA_SERIALIZE_FIELD_GET_SET(Bounciness, SetBounciness, "Bounciness", "Physics material bounciness");
				JIMARA_SERIALIZE_FIELD_GET_SET(BouncinessCombineMode, SetBouncinessCombineMode, 
					"Bounciness Combine", "Combine mode for the bounciness", CombineModeEnumAttribute());
			};
		}
	}
}

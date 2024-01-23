#include "PhysicsMaterial.h"
#include "../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	namespace Physics {
		void PhysicsMaterial::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				JIMARA_SERIALIZE_FIELD_GET_SET(StaticFriction, SetStaticFriction, "Static Friction", "Static friction ceofficient");
				JIMARA_SERIALIZE_FIELD_GET_SET(DynamicFriction, SetDynamicFriction, "Dynamic Friction", "Dynamic friction ceofficient");
				JIMARA_SERIALIZE_FIELD_GET_SET(Bounciness, SetBounciness, "Bounciness", "Physics material bounciness");
			};
		}
	}
}

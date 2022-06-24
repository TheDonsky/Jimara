#include "PhysicsInstance.h"
#include "../Data/Serialization/Attributes/EnumAttribute.h"


namespace Jimara {
	namespace Physics {
		const Object* DynamicBody::LockFlagMaskEnumAttribute() {
			static const Reference<const Serialization::EnumAttribute<LockFlagMask>> attribute = 
				Object::Instantiate<Serialization::EnumAttribute<LockFlagMask>>(true,
				"MOVEMENT_X", Physics::DynamicBody::LockFlag::MOVEMENT_X,
				"MOVEMENT_Y", Physics::DynamicBody::LockFlag::MOVEMENT_Y,
				"MOVEMENT_Z", Physics::DynamicBody::LockFlag::MOVEMENT_Z,
				"ROTATION_X", Physics::DynamicBody::LockFlag::ROTATION_X,
				"ROTATION_Y", Physics::DynamicBody::LockFlag::ROTATION_Y,
				"ROTATION_Z", Physics::DynamicBody::LockFlag::ROTATION_Z);
			return attribute;
		}
	}
}

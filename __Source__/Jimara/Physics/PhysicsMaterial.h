#pragma once
#include "../Core/Object.h"
#include "../Math/Math.h"


namespace Jimara {
	namespace Physics {
		class PhysicsMaterial : public virtual Object {
		public:
			virtual float StaticFriction()const = 0;

			virtual void SetStaticFriction(float friction) = 0;

			virtual float DynamicFriction()const = 0;

			virtual void SetDynamicFriction(float friction) = 0;

			virtual float Bounciness()const = 0;

			virtual void SetBounciness(float bounciness) = 0;
		};
	}
}

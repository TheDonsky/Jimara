#pragma once
#include "../Core/Object.h"
namespace Jimara {
	namespace Physics {
		class PhysicsScene;
		class PhysicsBody : public virtual Object {
		public:

		};
	}
}
#include "Collider.h"
#include "RigidBody.h"
#include "StaticBody.h"


namespace Jimara {
	namespace Physics {
		class PhysicsScene : public virtual Object {
		public:
			virtual Reference<RigidBody> AddRigidBody() = 0;

			virtual Reference<StaticBody> AddStaticBody() = 0;
		};
	}
}

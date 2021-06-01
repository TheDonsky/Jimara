#pragma once
#include "../Component.h"
#include "../Transform.h"


namespace Jimara {
	class Collider;

	class Rigidbody : public virtual Component {
	public:

	private:
		const Reference<Physics::RigidBody> m_body;

		friend class Collider;
	};
}

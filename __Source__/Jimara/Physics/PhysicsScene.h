#pragma once
#include "PhysicsCollider.h"
namespace Jimara {
	namespace Physics {
		class PhysicsScene;
		class PhysicsInstance;
	}
}
#include "RigidBody.h"
#include "StaticBody.h"
#include "PhysicsInstance.h"


namespace Jimara {
	namespace Physics {
		class PhysicsScene : public virtual Object {
		public:
			virtual Vector3 Gravity()const = 0;

			virtual void SetGravity(const Vector3& value) = 0;

			virtual Reference<RigidBody> AddRigidBody(const Matrix4& transform, bool enabled = true) = 0;

			virtual Reference<StaticBody> AddStaticBody(const Matrix4& transform, bool enabled = true) = 0;

			virtual void SimulateAsynch(float deltaTime) = 0;

			virtual void SynchSimulation() = 0;

			inline PhysicsInstance* APIInstance()const { return m_instance; }

		protected:
			PhysicsScene(PhysicsInstance* apiInstance) : m_instance(apiInstance) {}

		private:
			const Reference<PhysicsInstance> m_instance;
		};
	}
}

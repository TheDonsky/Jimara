#pragma once
#include "../Core/Object.h"
#include "../Math/Math.h"
namespace Jimara {
	namespace Physics {
		class PhysicsScene;
		class PhysicsInstance;
		class PhysicsBody : public virtual Object {
		public:
			virtual bool Active()const = 0;

			virtual void SetActive(bool active) = 0;

			virtual Matrix4 GetPose()const = 0;

			virtual void SetPose(const Matrix4& transform) = 0;
		};
	}
}
#include "Collider.h"
#include "RigidBody.h"
#include "StaticBody.h"
#include "PhysicsInstance.h"


namespace Jimara {
	namespace Physics {
		class PhysicsScene : public virtual Object {
		public:
			virtual Vector3 Gravity()const = 0;

			virtual void SetGravity(const Vector3& value) = 0;

			virtual Reference<RigidBody> AddRigidBody(const Matrix4& transform) = 0;

			virtual Reference<StaticBody> AddStaticBody(const Matrix4& transform) = 0;

			inline PhysicsInstance* APIInstance()const { return m_instance; }

		protected:
			PhysicsScene(PhysicsInstance* apiInstance) : m_instance(apiInstance) {}

		private:
			const Reference<PhysicsInstance> m_instance;
		};
	}
}

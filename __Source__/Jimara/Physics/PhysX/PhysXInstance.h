#pragma once
#include "../PhysicsInstance.h"
#include "PhysXAPIIncludes.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			class PhysXInstance : public virtual PhysicsInstance {
			public:
				PhysXInstance(OS::Logger* logger);

				virtual ~PhysXInstance();

				virtual Reference<PhysicsScene> CreateScene(size_t maxSimulationThreads, const Vector3 gravity) override;

				virtual Reference<PhysicsMaterial> CreateMaterial(float staticFriction, float dynamicFriction, float bounciness) override;

				operator physx::PxPhysics* () const;

				physx::PxPhysics* operator->()const;


			private:
				class ErrorCallback : public virtual physx::PxErrorCallback {
				public:
					ErrorCallback(OS::Logger* logger);

					virtual void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) override;

					OS::Logger* Log()const;

				private:
					const Reference<OS::Logger> m_logger;
				} m_errorCallback;
				physx::PxDefaultAllocator m_allocator;
				physx::PxFoundation* m_foundation = nullptr;
				physx::PxPvd* m_pvd = nullptr;
				physx::PxPhysics* m_physx = nullptr;
			};
		}
	}
}

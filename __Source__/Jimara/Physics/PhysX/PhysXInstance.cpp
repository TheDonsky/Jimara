#include "PhysXInstance.h"
#include "PhysXMaterial.h"
#include "PhysXCollisionMesh.h"
#include "PhysXScene.h"
#include "../../Core/Collections/ObjectCache.h"


#pragma warning(disable: 26812)
namespace Jimara {
	namespace Physics {
		namespace PhysX {
			namespace {
				class Instance : public virtual ObjectCache<uint8_t>::StoredObject {
				private:
					class ErrorCallback : public virtual physx::PxErrorCallback {
					private:
						const Reference<OS::Logger> m_logger;

					public:
						inline ErrorCallback(OS::Logger* logger) : m_logger(logger) {}

						inline virtual void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) override {
							std::stringstream stream;
							OS::Logger::LogLevel level = OS::Logger::LogLevel::LOG_DEBUG;
							if ((code & physx::PxErrorCode::eNO_ERROR) != 0) stream << "eNO_ERROR";
							if ((code & physx::PxErrorCode::eDEBUG_INFO) != 0)
								stream << (stream.str().length() > 0 ? "|" : "") << "eDEBUG_INFO";
							if ((code & physx::PxErrorCode::eDEBUG_WARNING) != 0) {
								stream << (stream.str().length() > 0 ? "|" : "") << "eDEBUG_WARNING";
								level = OS::Logger::LogLevel::LOG_WARNING;
							}
							if ((code & physx::PxErrorCode::ePERF_WARNING) != 0) {
								stream << (stream.str().length() > 0 ? "|" : "") << "ePERF_WARNING";
								level = OS::Logger::LogLevel::LOG_WARNING;
							}
							if ((code & physx::PxErrorCode::eINVALID_PARAMETER) != 0) {
								stream << (stream.str().length() > 0 ? "|" : "") << "eINVALID_PARAMETER";
								level = OS::Logger::LogLevel::LOG_ERROR;
							}
							if ((code & physx::PxErrorCode::eINVALID_OPERATION) != 0) {
								stream << (stream.str().length() > 0 ? "|" : "") << "eINVALID_OPERATION";
								level = OS::Logger::LogLevel::LOG_ERROR;
							}
							if ((code & physx::PxErrorCode::eOUT_OF_MEMORY) != 0) {
								stream << (stream.str().length() > 0 ? "|" : "") << "eOUT_OF_MEMORY";
								level = OS::Logger::LogLevel::LOG_ERROR;
							}
							if ((code & physx::PxErrorCode::eINTERNAL_ERROR) != 0) {
								stream << (stream.str().length() > 0 ? "|" : "") << "eINTERNAL_ERROR";
								level = OS::Logger::LogLevel::LOG_ERROR;
							}
							if ((code & physx::PxErrorCode::eABORT) != 0) {
								stream << (stream.str().length() > 0 ? "|" : "") << "eABORT";
								level = OS::Logger::LogLevel::LOG_FATAL;
							}
							m_logger->Log(level, "[PxErrorCodes: ", stream.str(), "] file: <", file, ">; line: ", line, "; message: <", message, ">");
						}
					} m_errorCallback;
					physx::PxDefaultAllocator m_allocator;
					physx::PxFoundation* m_foundation = nullptr;
					physx::PxPvd* m_pvd = nullptr;
					physx::PxPhysics* m_physx = nullptr;
					physx::PxCooking* m_cooking = nullptr;

				public:
					inline Instance(OS::Logger* logger) : m_errorCallback(logger) {
						m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_allocator, m_errorCallback);
						if (m_foundation == nullptr) {
							logger->Fatal("PhysXInstance - Failed to create foundation!");
							return;
						}

#ifndef NDEBUG
						m_pvd = physx::PxCreatePvd(*m_foundation);
						if (m_pvd == nullptr) {
							logger->Fatal("PhysXInstance - Failed to create PhysX visual debugger!");
							return;
						}

						physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
						if (transport == nullptr)
							logger->Error("PhysXInstance - Failed to create transport!");
						else m_pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);
#endif

						m_physx = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, physx::PxTolerancesScale(), true, m_pvd);
						if (m_physx == nullptr)
							logger->Fatal("PhysXInstance - Failed to create physX instance!");

						m_cooking = PxCreateCooking(PX_PHYSICS_VERSION, *m_foundation, physx::PxCookingParams(physx::PxTolerancesScale()));
						if (m_cooking == nullptr)
							logger->Fatal("PhysXInstance - Failed to create Cooking instance!");
					}

					virtual ~Instance() {
						if (m_cooking != nullptr) {
							m_cooking->release();
							m_cooking = nullptr;
						}
						if (m_physx != nullptr) {
							m_physx->release();
							m_physx = nullptr;
						}
						if (m_pvd != nullptr) {
							physx::PxPvdTransport* transport = m_pvd->getTransport();
							m_pvd->release();
							m_pvd = nullptr;
							if (transport != nullptr) transport->release();
						}
						if (m_foundation != nullptr) {
							m_foundation->release();
							m_foundation = nullptr;
						}
					}

					inline physx::PxPhysics* PhysX()const { return m_physx; }

					inline physx::PxCooking* Cooking()const { return m_cooking; }
				};

				class InstanceCache : public virtual ObjectCache<uint8_t> {
				public:
					inline static Reference<Instance> Get(OS::Logger* logger) {
						static InstanceCache cache;
						return cache.GetCachedOrCreate(0, false, [&]()->Reference<Instance> { return Object::Instantiate<Instance>(logger); });
					}
				};
			}

			PhysXInstance::PhysXInstance(OS::Logger* logger) : PhysicsInstance(logger), m_instance(InstanceCache::Get(logger)) {}

			Reference<PhysicsScene> PhysXInstance::CreateScene(size_t maxSimulationThreads, const Vector3 gravity) {
				return Object::Instantiate<PhysXScene>(this, maxSimulationThreads, gravity);
			}

			Reference<PhysicsMaterial> PhysXInstance::CreateMaterial(float staticFriction, float dynamicFriction, float bounciness) {
				return Object::Instantiate<PhysXMaterial>(this, staticFriction, dynamicFriction, bounciness);
			}

			Reference<CollisionMesh> PhysXInstance::CreateCollisionMesh(const TriMesh* mesh) {
				if (mesh == nullptr) {
					Log()->Error("PhysXInstance::CreateCollisionMesh - mesh missing!");
					return nullptr;
				}
				else return Object::Instantiate<PhysXCollisionMesh>(this, mesh);
			}

			PhysXInstance::operator physx::PxPhysics* () const { return dynamic_cast<Instance*>(m_instance.operator->())->PhysX(); }

			physx::PxPhysics* PhysXInstance::operator->()const { return dynamic_cast<Instance*>(m_instance.operator->())->PhysX(); }

			physx::PxCooking* PhysXInstance::Cooking()const { return dynamic_cast<Instance*>(m_instance.operator->())->Cooking(); }
		}
	}
}
#pragma warning(default: 26812)

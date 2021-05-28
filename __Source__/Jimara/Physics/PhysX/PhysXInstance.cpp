#include "PhysXInstance.h"
#include "PhysXScene.h"


#pragma warning(disable: 26812)
namespace Jimara {
	namespace Physics {
		namespace PhysX {
			PhysXInstance::PhysXInstance(OS::Logger* logger) : PhysicsInstance(logger), m_errorCallback(logger) {
				m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_allocator, m_errorCallback);
				if (m_foundation == nullptr) {
					logger->Fatal("PhysXInstance - Failed to create foundation!");
					return;
				}

				m_pvd = physx::PxCreatePvd(*m_foundation);
				if (m_pvd == nullptr) {
					logger->Fatal("PhysXInstance - Failed to create PhysX visual debugger!");
					return;
				}

				physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
				if (transport == nullptr)
					logger->Error("PhysXInstance - Failed to create transport!");
				else m_pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);

				m_physx = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, physx::PxTolerancesScale(), true, m_pvd);
				if (m_physx == nullptr)
					logger->Fatal("PhysXInstance - Failed to create physX instance!");
			}

			PhysXInstance::~PhysXInstance() {
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

			Reference<PhysicsScene> PhysXInstance::CreateScene(size_t maxSimulationThreads, const Vector3 gravity) {
				return Object::Instantiate<PhysXScene>(this, maxSimulationThreads, gravity);
			}

			PhysXInstance::operator physx::PxPhysics* () const { return m_physx; }

			physx::PxPhysics* PhysXInstance::operator->()const { return m_physx; }


			inline PhysXInstance::ErrorCallback::ErrorCallback(OS::Logger* logger) : m_logger(logger) {}

			inline void PhysXInstance::ErrorCallback::reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) {
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

			inline OS::Logger* PhysXInstance::ErrorCallback::Log()const { return m_logger; }
		}
	}
}
#pragma warning(default: 26812)

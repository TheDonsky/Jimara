#include "../GtestHeaders.h"
#include "../Memory.h"
#include "Shaders/SampleDiffuseShader.h"
#include "TestEnvironment/TestEnvironment.h"
#include "Components/MeshRenderer.h"
#include "Components/Lights/PointLight.h"
#include "Components/Lights/DirectionalLight.h"
#include <sstream>

#pragma warning(disable: 26812)
#pragma warning(disable: 26495)
#pragma warning(disable: 26451)
#pragma warning(disable: 33010)

#define PX_PHYSX_STATIC_LIB

#ifdef _DEBUG
 #ifdef NDEBUG
  #define PX_INCLUDES_NDEBUG_WAS_DEFINED
  #undef NDEBUG
 #endif
#else
 #ifndef NDEBUG
  #define PX_INCLUDES_NDEBUG_WAS_NOT_DEFINED
  #define NDEBUG
 #endif
#endif
#include "PxPhysicsAPI.h"
#include "PxPhysicsVersion.h"
#ifdef PX_INCLUDES_NDEBUG_WAS_DEFINED
 #undef PX_INCLUDES_NDEBUG_WAS_DEFINED
 #define NDEBUG
#else
 #ifdef PX_INCLUDES_NDEBUG_WAS_NOT_DEFINED
  #undef PX_INCLUDES_NDEBUG_WAS_NOT_DEFINED
  #undef NDEBUG
 #endif
#endif


namespace Jimara {
	namespace {
		class PhysicXFoundation : public virtual Object {
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

				inline OS::Logger* Log()const { return m_logger; }
			} m_errorCallback;
			physx::PxDefaultAllocator m_allocator;
			physx::PxFoundation* m_foundation = nullptr;
			physx::PxPvd* m_pvd = nullptr;
			physx::PxPhysics* m_physx = nullptr;

		public:
			inline PhysicXFoundation(OS::Logger* logger) : m_errorCallback(logger) {
				m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_allocator, m_errorCallback);
				if (m_foundation == nullptr) {
					logger->Fatal("PhysicXFoundation - Failed to create foundation!");
					return;
				}
				
				m_pvd = physx::PxCreatePvd(*m_foundation);
				if (m_pvd == nullptr) {
					logger->Fatal("PhysicXFoundation - Failed to create PhysX visual debugger!");
					return;
				}
				
				physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
				if (transport == nullptr)
					logger->Error("PhysicXFoundation - Failed to create transport!");
				else m_pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);

				m_physx = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, physx::PxTolerancesScale(), true, m_pvd);
				if (m_physx == nullptr)
					logger->Fatal("PhysicXFoundation - Failed to create physX instance!");
			}

			inline virtual ~PhysicXFoundation() {
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

			inline OS::Logger* Log()const { return m_errorCallback.Log(); }

			inline physx::PxFoundation* Foundation()const { return m_foundation; }

			inline physx::PxPhysics* PhysX()const { return m_physx; }
		};

		class PhysicXScene : public virtual Object {
		private:
			const Reference<PhysicXFoundation> m_foundation;
			physx::PxDefaultCpuDispatcher* m_dispatcher = nullptr;
			physx::PxScene* m_scene = nullptr;

		public:
			inline PhysicXScene(PhysicXFoundation* foundation) : m_foundation(foundation) {
				m_dispatcher = physx::PxDefaultCpuDispatcherCreate(max(std::thread::hardware_concurrency() / 4u, 1u));
				if (m_dispatcher == nullptr) {
					m_foundation->Log()->Fatal("PhysicXScene - Failed to create the dispatcher!");
				}
				
				physx::PxSceneDesc sceneDesc(m_foundation->PhysX()->getTolerancesScale());
				sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
				sceneDesc.cpuDispatcher = m_dispatcher;
				sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
				m_scene = m_foundation->PhysX()->createScene(sceneDesc);
				if(m_scene == nullptr)
					m_foundation->Log()->Fatal("PhysicXScene - Failed to create the scene!");
			}

			inline virtual ~PhysicXScene() {
				if (m_scene != nullptr) {
					m_scene->release();
					m_scene = nullptr;
				}
				if (m_dispatcher != nullptr) {
					m_dispatcher->release();
					m_dispatcher = nullptr;
				}
			}

			physx::PxScene* Scene()const { return m_scene; }
		};
	}

	TEST(PhysicsPlayground, Playground) {
		Jimara::Test::TestEnvironment environment("PhysicsPlayground");
		Reference<PhysicXFoundation> foundation = Object::Instantiate<PhysicXFoundation>(environment.RootObject()->Context()->Log());
		Reference<PhysicXScene> scene = Object::Instantiate<PhysicXScene>(foundation);
	}
}


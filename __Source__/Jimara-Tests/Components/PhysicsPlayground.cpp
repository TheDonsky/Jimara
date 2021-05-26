#include "../GtestHeaders.h"
#include "../Memory.h"
#include "Shaders/SampleDiffuseShader.h"
#include "TestEnvironment/TestEnvironment.h"
#include "Components/MeshRenderer.h"
#include "Components/Lights/PointLight.h"
#include "Components/Lights/DirectionalLight.h"
#include <sstream>

#ifdef _WIN32
#pragma warning(disable: 26812)
#pragma warning(disable: 26495)
#pragma warning(disable: 26451)
#pragma warning(disable: 33010)
#include "PxPhysicsAPI.h"
#include "PxPhysicsVersion.h"


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
			} m_errorCallback;
			physx::PxDefaultAllocator m_allocator;
			physx::PxFoundation* m_foundation = nullptr;

		public:
			inline PhysicXFoundation(OS::Logger* logger) : m_errorCallback(logger) {
				m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_allocator, m_errorCallback);
				if (m_foundation == nullptr) logger->Fatal("PhysicXFoundation - Failed to create foundation!");
			}

			inline virtual ~PhysicXFoundation() {
				if (m_foundation != nullptr) {
					m_foundation->release();
					m_foundation = nullptr;
				}
			}
		};
	}

	TEST(PhysicsPlayground, Playground) {
		Jimara::Test::TestEnvironment environment("PhysicsPlayground");
		Reference<PhysicXFoundation> foundation = Object::Instantiate<PhysicXFoundation>(environment.RootObject()->Context()->Log());
	}
}

#endif

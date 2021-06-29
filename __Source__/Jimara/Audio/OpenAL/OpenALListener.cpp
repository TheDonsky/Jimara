#include "OpenALListener.h"



namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			namespace {
				static const OS::Logger::LogLevel FATAL = OS::Logger::LogLevel::LOG_FATAL, WARNING = OS::Logger::LogLevel::LOG_WARNING;
			}

			OpenALContext::OpenALContext(ALCdevice* device, OpenALInstance* instance, const Object* deviceHolder) : m_instance(instance), m_deviceHolder(deviceHolder) {
				std::unique_lock<std::mutex> lock(OpenALInstance::APILock());
				m_context = alcCreateContext(device, nullptr);
				if (m_instance->ReportALCError("OpenALContext::OpenALContext - alcCreateContext(*device, nullptr) Failed!", FATAL) >= WARNING) return;
				else if (m_context == nullptr) m_instance->Log()->Fatal("OpenALContext::OpenALContext - Failed to create context!");
			}

			OpenALContext::~OpenALContext() {
				if (m_context != nullptr) {
					std::unique_lock<std::mutex> lock(OpenALInstance::APILock());
					alcDestroyContext(m_context);
					if (m_instance->ReportALCError("OpenALContext::OpenALContext - alcCreateContext(*device, nullptr) Failed!", FATAL) >= WARNING) return;
					m_context = nullptr;
				}
			}

			OpenALContext::operator ALCcontext* ()const { return m_context; }


			OpenALContext::SwapCurrent::SwapCurrent(const OpenALContext* context) 
				: m_context(context), m_old(alcGetCurrentContext()) {
				if (context->m_instance->ReportALCError("OpenALContext::SwapCurrent::SwapCurrent - alcGetCurrentContext() Failed!", FATAL) >= WARNING) return;
				else if ((*m_context) == m_old) return;
				ALCboolean success = alcMakeContextCurrent(*context);
				if (context->m_instance->ReportALCError("OpenALContext::SwapCurrent::SwapCurrent - alcMakeContextCurrent(*context) Failed!", FATAL) >= WARNING) return;
				else if (success != AL_TRUE) context->m_instance->Log()->Fatal("OpenALContext::SwapCurrent::SwapCurrent - alcMakeContextCurrent(*context) returned false!");
			}

			OpenALContext::SwapCurrent::~SwapCurrent() {
				if ((*m_context) == m_old) return;
				ALCboolean success = alcMakeContextCurrent(m_old);
				if (m_context->m_instance->ReportALCError("OpenALContext::SwapCurrent::~SwapCurrent - alcMakeContextCurrent(m_old) Failed!", FATAL) >= WARNING) return;
				else if (success != AL_TRUE) m_context->m_instance->Log()->Fatal("OpenALContext::SwapCurrent::~SwapCurrent - alcMakeContextCurrent(m_old) returned false!");
			}



			OpenALListener::OpenALListener(const Settings& settings, OpenALScene* scene)
				: OpenALContext(*dynamic_cast<OpenALDevice*>(scene->Device()), dynamic_cast<OpenALInstance*>(scene->Device()->APIInstance()), scene)
				, m_scene(scene) {
				Update(settings);
			}

			void OpenALListener::Update(const Settings& newSettings) {
				const Vector3 position = newSettings.pose[3];
				const Vector3 forward = Math::Normalize((Vector3)newSettings.pose[2]);
				const Vector3 up = Math::Normalize((Vector3)newSettings.pose[1]);
				const Vector3& velocity = newSettings.velocity;
				
				const ALfloat orientationData[] = {
					static_cast<ALfloat>(forward.x), static_cast<ALfloat>(forward.y), static_cast<ALfloat>(-forward.z),
					static_cast<ALfloat>(up.x), static_cast<ALfloat>(up.y), static_cast<ALfloat>(-up.z)
				};

				OpenALDevice* const device = dynamic_cast<OpenALDevice*>(m_scene->Device());

				std::unique_lock<std::mutex> lock(OpenALInstance::APILock());
				SwapCurrent swap(this);
				
				alListenerf(AL_GAIN, static_cast<ALfloat>(newSettings.volume));
				device->ALInstance()->ReportALError("OpenALListener::Update - alListenerf(AL_GAIN, volume) failed!");

				alListener3f(AL_POSITION, static_cast<ALfloat>(position.x), static_cast<ALfloat>(position.y), static_cast<ALfloat>(-position.z));
				device->ALInstance()->ReportALError("OpenALListener::Update - alListenerf(AL_POSITION, position) failed!");

				alListener3f(AL_VELOCITY, static_cast<ALfloat>(velocity.x), static_cast<ALfloat>(velocity.y), static_cast<ALfloat>(-velocity.z));
				device->ALInstance()->ReportALError("OpenALListener::Update - alListenerf(AL_VELOCITY, velocity) failed!");

				alListenerfv(AL_ORIENTATION, orientationData);
				device->ALInstance()->ReportALError("OpenALListener::Update - alListenerf(AL_ORIENTATION, forward, up) failed!");
			}
		}
	}
}



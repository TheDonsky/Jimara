#include "OpenALListener.h"



namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			OpenALContext::OpenALContext(ALCdevice* device, OpenALInstance* instance, const Object* deviceHolder) : m_instance(instance), m_deviceHolder(deviceHolder) {
				std::unique_lock<std::mutex> lock(OpenALInstance::APILock());
				m_context = alcCreateContext(device, nullptr);
				if (m_instance->ReportALCError("OpenALContext::OpenALContext - alcCreateContext(*device, nullptr) Failed!") >= OS::Logger::LogLevel::LOG_WARNING) return;
				else if (m_context == nullptr) m_instance->Log()->Fatal("OpenALContext::OpenALContext - Failed to create context!");
			}

			OpenALContext::~OpenALContext() {
				if (m_context != nullptr) {
					std::unique_lock<std::mutex> lock(OpenALInstance::APILock());
					alcDestroyContext(m_context);
					if (m_instance->ReportALCError("OpenALContext::OpenALContext - alcCreateContext(*device, nullptr) Failed!") >= OS::Logger::LogLevel::LOG_WARNING) return;
					m_context = nullptr;
				}
			}

			OpenALContext::operator ALCcontext* ()const { return m_context; }


			OpenALContext::SwapCurrent::SwapCurrent(const OpenALContext* context) 
				: m_context(context), m_old(alcGetCurrentContext()) {
				if (context->m_instance->ReportALCError("OpenALContext::SwapCurrent::SwapCurrent - alcGetCurrentContext() Failed!") >= OS::Logger::LogLevel::LOG_WARNING) return;
				else if ((*m_context) == m_old) return;
				ALCboolean success = alcMakeContextCurrent(*context);
				if (context->m_instance->ReportALCError("OpenALContext::SwapCurrent::SwapCurrent - alcMakeContextCurrent(*context) Failed!") >= OS::Logger::LogLevel::LOG_WARNING) return;
				else if (success != AL_TRUE) context->m_instance->Log()->Fatal("OpenALContext::SwapCurrent::SwapCurrent - alcMakeContextCurrent(*context) returned false!");
			}

			OpenALContext::SwapCurrent::~SwapCurrent() {
				if ((*m_context) == m_old) return;
				ALCboolean success = alcMakeContextCurrent(m_old);
				if (m_context->m_instance->ReportALCError("OpenALContext::SwapCurrent::~SwapCurrent - alcMakeContextCurrent(m_old) Failed!") >= OS::Logger::LogLevel::LOG_WARNING) return;
				else if (success != AL_TRUE) m_context->m_instance->Log()->Fatal("OpenALContext::SwapCurrent::~SwapCurrent - alcMakeContextCurrent(m_old) returned false!");
			}


			ListenerContext::ListenerContext(OpenALDevice* device)
				: OpenALContext(*device, device->ALInstance(), device)
				, m_device(device)
				, m_sources(device->MaxSources()) {
				{
					std::unique_lock<std::mutex> lock(OpenALInstance::APILock());
					SwapCurrent swap(this);
					alGenSources(static_cast<ALsizei>(m_sources.size()), m_sources.data());
					if (m_device->ALInstance()->ReportALError("ListenerContext::ListenerContext - alGenSources() Failed!") > Jimara::OS::Logger::LogLevel::LOG_WARNING) {
						m_sources.clear();
						return;
					}
				}
				for (size_t i = 0; i < m_sources.size(); i++) m_freeSources.push(m_sources[i]);
			}

			ListenerContext::~ListenerContext() {
				m_freeSources = std::stack<ALuint>();
				if (m_sources.size() > 0u) {
					std::unique_lock<std::mutex> lock(OpenALInstance::APILock());
					SwapCurrent swap(this);
					alDeleteSources(static_cast<ALsizei>(m_sources.size()), m_sources.data());
					m_device->ALInstance()->ReportALError("ListenerContext::~ListenerContext - alDeleteSources() Failed!");
				}
				m_sources.clear();
			}

			ALuint ListenerContext::GetSource() {
				std::unique_lock<std::mutex> lock;
				if (m_freeSources.size() <= 0u) {
					m_device->ALInstance()->Log()->Fatal("ListenerContext::GetSource - No free sources available!");
					return 0;
				}
				ALuint source = m_freeSources.top();
				m_freeSources.pop();
				return source;
			}

			void ListenerContext::FreeSource(ALuint source) {
				std::unique_lock<std::mutex> lock;
				m_freeSources.push(source);
			}

			OpenALDevice* ListenerContext::Device()const { return m_device; }



			OpenALListener::OpenALListener(const Settings& settings, OpenALScene* scene)
				: m_context(Object::Instantiate<ListenerContext>(dynamic_cast<OpenALDevice*>(scene->Device())))
				, m_scene(scene) {
				Update(settings);
			}
			
			OpenALListener::~OpenALListener() {
				if (m_volume > 0.0f) m_scene->RemoveListenerContext(m_context);
			}

			void OpenALListener::Update(const Settings& newSettings) {
				const Vector3 position = newSettings.pose[3];
				const Vector3 forward = Math::Normalize((Vector3)newSettings.pose[2]);
				const Vector3 up = Math::Normalize((Vector3)newSettings.pose[1]);
				const Vector3& velocity = newSettings.velocity;

				std::unique_lock<std::mutex> m_upateLock;
				{
					const ALfloat orientationData[] = {
						static_cast<ALfloat>(forward.x), static_cast<ALfloat>(forward.y), static_cast<ALfloat>(-forward.z),
						static_cast<ALfloat>(up.x), static_cast<ALfloat>(up.y), static_cast<ALfloat>(-up.z)
					};
					OpenALDevice* const device = dynamic_cast<OpenALDevice*>(m_scene->Device());

					std::unique_lock<std::mutex> lock(OpenALInstance::APILock());
					ListenerContext::SwapCurrent swap(m_context);

					alListenerf(AL_GAIN, static_cast<ALfloat>(newSettings.volume));
					device->ALInstance()->ReportALError("OpenALListener::Update - alListenerf(AL_GAIN, volume) failed!");

					alListener3f(AL_POSITION, static_cast<ALfloat>(position.x), static_cast<ALfloat>(position.y), static_cast<ALfloat>(-position.z));
					device->ALInstance()->ReportALError("OpenALListener::Update - alListenerf(AL_POSITION, position) failed!");

					alListener3f(AL_VELOCITY, static_cast<ALfloat>(velocity.x), static_cast<ALfloat>(velocity.y), static_cast<ALfloat>(-velocity.z));
					device->ALInstance()->ReportALError("OpenALListener::Update - alListenerf(AL_VELOCITY, velocity) failed!");

					alListenerfv(AL_ORIENTATION, orientationData);
					device->ALInstance()->ReportALError("OpenALListener::Update - alListenerf(AL_ORIENTATION, forward, up) failed!");
				}

				bool active = (newSettings.volume > 0.0f);
				if ((m_volume > 0.0f) != active) {
					if (active) m_scene->AddListenerContext(m_context);
					else m_scene->RemoveListenerContext(m_context);
				}
				m_volume = newSettings.volume;
			}


			ListenerContext* OpenALListener::Context()const { return m_context; }
		}
	}
}



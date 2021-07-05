#include "OpenALContext.h"

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
				, m_device(device) {}

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
					if (m_sources.size() >= m_device->MaxSources()) {
						m_device->ALInstance()->Log()->Fatal("ListenerContext::GetSource - No free sources available!");
						return 0;
					}
					size_t initialSize = m_sources.size();
					m_sources.resize(min(m_sources.size() * 2 + 1, m_device->MaxSources()));
					{
						std::unique_lock<std::mutex> apiLock(OpenALInstance::APILock());
						SwapCurrent swap(this);
						alGenSources(static_cast<ALsizei>(m_sources.size() - initialSize), m_sources.data() + initialSize);
						if (m_device->ALInstance()->ReportALError("ListenerContext::GetSource - alGenSources() Failed!") > Jimara::OS::Logger::LogLevel::LOG_WARNING) {
							m_sources.resize(initialSize);
							m_device->ALInstance()->Log()->Fatal("ListenerContext::GetSource - Failed!");
							return 0;
						}
					}
					for (size_t i = initialSize; i < m_sources.size(); i++) m_freeSources.push(m_sources[i]);
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
		}
	}
}

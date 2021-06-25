#include "OpenALListener.h"



namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			namespace {
				static const OS::Logger::LogLevel FATAL = OS::Logger::LogLevel::LOG_FATAL, WARNING = OS::Logger::LogLevel::LOG_WARNING;
			}

			OpenALContext::OpenALContext(ALCdevice* device, OpenALInstance* instance) : m_instance(instance) {
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
		}
	}
}



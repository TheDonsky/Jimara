#include "OpenALClip.h"
#include "../../Math/Math.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			namespace {
				static const OS::Logger::LogLevel FATAL = OS::Logger::LogLevel::LOG_FATAL, WARNING = OS::Logger::LogLevel::LOG_WARNING;
			}

			OpenALClip::OpenALClip(OpenALDevice* device, const AudioBuffer* buffer) 
				: AudioClip(buffer), m_device(device) {
				if (buffer->ChannelCount() > 2 || buffer->ChannelCount() <= 0)
					m_device->ALInstance()->Log()->Fatal("OpenALClip::OpenALClip - buffer with ", buffer->ChannelCount(), " channels not[yet] supported!");
				
				std::vector<int16_t> bufferData(buffer->ChannelCount() * buffer->SampleCount());
				{
					AudioData data(buffer->ChannelCount(), buffer->SampleCount());
					buffer->GetData(0, data.SampleCount(), data);
					size_t index = 0;
					float MX = static_cast<float>(SHRT_MAX);
					float MN = static_cast<float>(SHRT_MIN);
					for (size_t i = 0; i < data.SampleCount(); i++)
						for (size_t channel = 0; channel < data.ChannelCount(); channel++) {
							bufferData[index] = static_cast<int16_t>(min(max(MN, data(channel, i) * MX), MX));
							index++;
						}
				}
				const ALenum format = (buffer->ChannelCount() > 1) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;

				std::unique_lock<std::mutex> lock(OpenALInstance::APILock());
				OpenALContext::SwapCurrent setContext(m_device->DefaultContext());
				alGenBuffers(1, &m_buffer);
				if (m_device->ALInstance()->ReportALError("OpenALClip::OpenALClip - alGenBuffers(1, &m_buffer) Failed!", FATAL) >= WARNING) {
					m_buffer = 0;
					return;
				}
				m_bufferPresent = true;
				alBufferData(m_buffer, format, bufferData.data(), static_cast<ALsizei>(sizeof(int16_t) * bufferData.size()), static_cast<ALsizei>(buffer->SampleRate()));
				if (m_device->ALInstance()->ReportALError("OpenALClip::OpenALClip - alBufferData(...) Failed!", FATAL) >= WARNING) return;
			}

			OpenALClip::~OpenALClip() {
				if (m_bufferPresent) {
					std::unique_lock<std::mutex> lock(OpenALInstance::APILock());
					OpenALContext::SwapCurrent setContext(m_device->DefaultContext());
					alDeleteBuffers(1, &m_buffer);
					if (m_device->ALInstance()->ReportALError("OpenALClip::~OpenALClip - alDeleteBuffers(1, &m_buffer) Failed!", FATAL) >= WARNING) return;
					m_bufferPresent = false;
					m_buffer = 0;
				}
			}

			ALuint OpenALClip::Buffer()const { return m_buffer; }
		}
	}
}

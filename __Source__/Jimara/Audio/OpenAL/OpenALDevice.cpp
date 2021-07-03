#include "OpenALDevice.h"
#include "OpenALScene.h"
#include "OpenALClip.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			OpenALDevice::OpenALDevice(OpenALInstance* instance, PhysicalAudioDevice* physicalDevice) : AudioDevice(instance, physicalDevice) {
				{
					std::unique_lock<std::mutex> lock(OpenALInstance::APILock());
					m_device = alcOpenDevice(PhysicalDevice()->Name().c_str());
					if (ALInstance()->ReportALCError("OpenALDevice::OpenALDevice - alcOpenDevice(PhysicalDevice()->Name().c_str()) Failed!") > OS::Logger::LogLevel::LOG_WARNING) return;
					else if (m_device == nullptr) {
						APIInstance()->Log()->Fatal("OpenALDevice::OpenALDevice - Failed to open device!");
						return;
					}
					ALCint monoSources = 0, stereoSources = 0;
					alcGetIntegerv(m_device, ALC_MONO_SOURCES, 1, &monoSources);
					if ((ALInstance()->ReportALCError("OpenALDevice::OpenALDevice - alcGetIntegerv(m_device, ALC_MONO_SOURCES, 1, &monoSources) Failed!") > OS::Logger::LogLevel::LOG_WARNING) || monoSources <= 0) {
						ALInstance()->Log()->Warning("OpenALDevice::OpenALDevice - m_maxMonoSources defaulted to 32");
						monoSources = 32;
					}
					alcGetIntegerv(m_device, ALC_STEREO_SOURCES, 1, &stereoSources);
					if ((ALInstance()->ReportALCError("OpenALDevice::OpenALDevice - alcGetIntegerv(m_device, ALC_STEREO_SOURCES, 1, &stereoSources) Failed!") > OS::Logger::LogLevel::LOG_WARNING) || stereoSources <= 0) {
						ALInstance()->Log()->Warning("OpenALDevice::OpenALDevice - m_maxStereoSources defaulted to 32");
						stereoSources = 0;
					}
					m_maxSources = static_cast<size_t>(monoSources) + stereoSources;
				}
				m_defaultContext = Object::Instantiate<OpenALContext>(m_device, ALInstance(), nullptr);
			}

			OpenALDevice::~OpenALDevice() {
				m_defaultContext = nullptr;
				std::unique_lock<std::mutex> lock(OpenALInstance::APILock());
				if (m_device != nullptr) {
					alcCloseDevice(m_device);
					m_device = nullptr;
					ALInstance()->ReportALCError("OpenALDevice::~OpenALDevice - alcCloseDevice(m_device); Failed!");
				}
			}

			Reference<AudioScene> OpenALDevice::CreateScene() {
				return Object::Instantiate<OpenALScene>(this);
			}

			Reference<AudioClip> OpenALDevice::CreateAudioClip(const AudioBuffer* buffer, bool streamed) {
				return OpenALClip::Create(this, buffer, streamed);
			}

			OpenALInstance* OpenALDevice::ALInstance()const { 
				return dynamic_cast<OpenALInstance*>(APIInstance()); 
			}

			OpenALDevice::operator ALCdevice* ()const { return m_device; }

			OpenALContext* OpenALDevice::DefaultContext()const { return m_defaultContext; }

			size_t OpenALDevice::MaxSources()const { return m_maxSources; }
		}
	}
}



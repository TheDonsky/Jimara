#include "OpenALDevice.h"
#include "OpenALScene.h"
#include "OpenALClip.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			namespace {
				static const OS::Logger::LogLevel FATAL = OS::Logger::LogLevel::LOG_FATAL, WARNING = OS::Logger::LogLevel::LOG_WARNING;
			}

			OpenALDevice::OpenALDevice(OpenALInstance* instance, PhysicalAudioDevice* physicalDevice) : AudioDevice(instance, physicalDevice) {
				{
					std::unique_lock<std::mutex> lock(OpenALInstance::APILock());
					m_device = alcOpenDevice(PhysicalDevice()->Name().c_str());
					if (ALInstance()->ReportALCError("OpenALDevice::OpenALDevice - alcOpenDevice(PhysicalDevice()->Name().c_str()) Failed!", FATAL) > WARNING) return;
					else if (m_device == nullptr) {
						APIInstance()->Log()->Fatal("OpenALDevice::OpenALDevice - Failed to open device!");
						return;
					}
					ALCint monoSources = 0, stereoSources = 0;
					alcGetIntegerv(m_device, ALC_MONO_SOURCES, 1, &monoSources);
					if ((ALInstance()->ReportALCError("OpenALDevice::OpenALDevice - alcGetIntegerv(m_device, ALC_MONO_SOURCES, 1, &monoSources) Failed!") > WARNING) || monoSources <= 0) {
						ALInstance()->Log()->Warning("OpenALDevice::OpenALDevice - m_maxMonoSources defaulted to 32");
						m_maxMonoSources = 32u;
					}
					else m_maxMonoSources = static_cast<size_t>(monoSources);
					alcGetIntegerv(m_device, ALC_STEREO_SOURCES, 1, &stereoSources);
					if ((ALInstance()->ReportALCError("OpenALDevice::OpenALDevice - alcGetIntegerv(m_device, ALC_STEREO_SOURCES, 1, &stereoSources) Failed!") > WARNING) || stereoSources <= 0) {
						ALInstance()->Log()->Warning("OpenALDevice::OpenALDevice - m_maxStereoSources defaulted to 32");
						m_maxStereoSources = 32u;
					}
					else m_maxStereoSources = static_cast<size_t>(stereoSources);
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
				if (buffer == nullptr) {
					APIInstance()->Log()->Error("OpenALDevice::CreateAudioClip - null buffer provided!");
					return nullptr;
				}
				return Object::Instantiate<OpenALClip>(this, buffer);
			}

			OpenALInstance* OpenALDevice::ALInstance()const { 
				return dynamic_cast<OpenALInstance*>(APIInstance()); 
			}

			OpenALDevice::operator ALCdevice* ()const { return m_device; }

			OpenALContext* OpenALDevice::DefaultContext()const { return m_defaultContext; }

			size_t OpenALDevice::MaxMonoSources()const { return m_maxMonoSources; }

			size_t OpenALDevice::MaxStereoSources()const { return m_maxStereoSources; }
		}
	}
}



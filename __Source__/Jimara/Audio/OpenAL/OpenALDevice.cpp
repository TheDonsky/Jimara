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
					if (ALInstance()->ReportALCError("OpenALDevice::OpenALDevice - alcOpenDevice(PhysicalDevice()->Name().c_str()) Failed!", FATAL) >= WARNING) return;
					else if (m_device == nullptr) APIInstance()->Log()->Fatal("OpenALDevice::OpenALDevice - Failed to open device!");
				}
				m_defaultContext = Object::Instantiate<OpenALContext>(m_device, ALInstance());
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
		}
	}
}



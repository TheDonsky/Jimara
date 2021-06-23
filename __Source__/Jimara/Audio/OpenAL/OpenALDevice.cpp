#include "OpenALDevice.h"
#include "OpenALScene.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			OpenALDevice::OpenALDevice(OpenALInstance* instance, PhysicalAudioDevice* physicalDevice) : AudioDevice(instance, physicalDevice) {
				std::unique_lock<std::mutex> lock(OpenALInstance::APILock());
				m_device = alcOpenDevice(PhysicalDevice()->Name().c_str());
				const OS::Logger::LogLevel FATAL = OS::Logger::LogLevel::LOG_FATAL, WARNING = OS::Logger::LogLevel::LOG_WARNING;
				if (ALInstance()->ReportALCError("OpenALDevice::OpenALDevice - alcOpenDevice(PhysicalDevice()->Name().c_str()) Failed!", FATAL) >= WARNING) return;
				else if (m_device == nullptr) APIInstance()->Log()->Fatal("OpenALDevice::OpenALDevice - Failed to open device!");
			}

			OpenALDevice::~OpenALDevice() {
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

			OpenALInstance* OpenALDevice::ALInstance()const { 
				return dynamic_cast<OpenALInstance*>(APIInstance()); 
			}

			OpenALDevice::operator ALCdevice* ()const { return m_device; }
		}
	}
}



#include "OpenALInstance.h"
#include "OpenALDevice.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			OpenALInstance::OpenALInstance(OS::Logger* logger) : AudioInstance(logger) {
				std::unique_lock<std::mutex> lock(APILock());
				alcGetError(nullptr);
				bool allExtPresent = alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT");
				if (ReportALCError("OpenALInstance - alcIsExtensionPresent(nullptr, \"ALC_ENUMERATE_ALL_EXT\") Failed!") > OS::Logger::LogLevel::LOG_WARNING) return;
				{
					const ALCchar* devices = alcGetString(nullptr, allExtPresent ? ALC_ALL_DEVICES_SPECIFIER : ALC_DEVICE_SPECIFIER);
					if (ReportALCError("OpenALInstance - alcGetString(nullptr, allExtPresent ? ALC_ALL_DEVICES_SPECIFIER : ALC_DEVICE_SPECIFIER) Failed!") > OS::Logger::LogLevel::LOG_WARNING) return;
					std::vector<std::string> names;
					while (devices != nullptr && devices[0] != '\0') {
						names.push_back(devices);
						devices += names.back().size() + 1;
					}
					if (names.size() > 0u) {
						m_deviceCount = names.size();
						m_devices = new OpenALPhysicalDevice[m_deviceCount];
						for (size_t i = 0; i < m_deviceCount; i++) {
							OpenALPhysicalDevice& device = m_devices[i];
							device.m_instance = this;
							device.m_name = names[i];
							device.m_index = i;
							device.ReleaseRef();
						}
					}
					else Log()->Fatal("OpenALInstance - No Physical devices found!");
				}
				{
					const ALCchar* defaultDeviceName = alcGetString(nullptr, allExtPresent ? ALC_DEFAULT_ALL_DEVICES_SPECIFIER : ALC_DEFAULT_DEVICE_SPECIFIER);
					if (ReportALCError("OpenALInstance - alcGetString(nullptr, allExtPresent ? ALC_DEFAULT_ALL_DEVICES_SPECIFIER : ALC_DEFAULT_DEVICE_SPECIFIER) Failed!") > OS::Logger::LogLevel::LOG_WARNING) return;
					else for (size_t i = 0; i < m_deviceCount; i++)
						if (m_devices[i].Name() == defaultDeviceName) {
							m_defaultDeviceId = i;
							break;
						}
				}
				m_tickThread = std::thread([](OpenALInstance* self) {
					Stopwatch stopwatch;
					SynchronousActionQueue queue;
					while (!self->m_killTick) {
						float deltaTime = stopwatch.Reset();
						self->m_onTick(deltaTime, queue);
						queue.Flush();
						std::this_thread::sleep_for(std::chrono::milliseconds(8));
					}
					}, this);
			}

			OpenALInstance::~OpenALInstance() {
				m_killTick = true;
				m_tickThread.join();
				if (m_devices != nullptr) {
					delete[] m_devices;
					m_devices = nullptr;
				}
				m_deviceCount = 0;
				m_defaultDeviceId = 0;
			}

			size_t OpenALInstance::PhysicalDeviceCount()const { return m_deviceCount; }

			Reference<PhysicalAudioDevice> OpenALInstance::PhysicalDevice(size_t index)const {
				OpenALPhysicalDevice* alDevice = &m_devices[index];
				Reference<PhysicalAudioDevice> device(alDevice);
				alDevice->m_owner = this;
				return device;
			}

			size_t OpenALInstance::DefaultDeviceId()const { return m_defaultDeviceId; }

			OS::Logger::LogLevel OpenALInstance::Report(ALenum error, const char* message)const {
				const char* errorType = nullptr;
				OS::Logger::LogLevel errorLevel;
				if (error == AL_NO_ERROR) {
					static const char ERR_NAME[] = "AL_NO_ERROR";
					errorType = ERR_NAME;
					errorLevel = OS::Logger::LogLevel::LOG_INFO;
				}
				else if (error == AL_INVALID_NAME) {
					static const char ERR_NAME[] = "AL_INVALID_NAME";
					errorType = ERR_NAME;
					errorLevel = OS::Logger::LogLevel::LOG_WARNING;
				}
				else if (error == AL_INVALID_ENUM) {
					static const char ERR_NAME[] = "AL_INVALID_ENUM";
					errorType = ERR_NAME;
					errorLevel = OS::Logger::LogLevel::LOG_WARNING;
				}
				else if (error == AL_INVALID_VALUE) {
					static const char ERR_NAME[] = "AL_INVALID_VALUE";
					errorType = ERR_NAME;
					errorLevel = OS::Logger::LogLevel::LOG_FATAL;
				}
				else if (error == AL_INVALID_OPERATION) {
					static const char ERR_NAME[] = "AL_INVALID_OPERATION";
					errorType = ERR_NAME;
					errorLevel = OS::Logger::LogLevel::LOG_FATAL;
				}
				else if (error == AL_OUT_OF_MEMORY) {
					static const char ERR_NAME[] = "AL_OUT_OF_MEMORY";
					errorType = ERR_NAME;
					errorLevel = OS::Logger::LogLevel::LOG_FATAL;
				}
				else {
					static const char ERR_NAME[] = "<unknown AL error>";
					errorType = ERR_NAME;
					errorLevel = OS::Logger::LogLevel::LOG_FATAL;
				}
				return errorLevel;
			}

			OS::Logger::LogLevel OpenALInstance::ReportALCError(const char* message)const {
				ALenum error = alcGetError(nullptr);
				if (error != AL_NO_ERROR)
					return Report(error, message);
				else return OS::Logger::LogLevel::LOG_DEBUG;
			}

			OS::Logger::LogLevel OpenALInstance::ReportALError(const char* message)const {
				ALenum error = alGetError();
				if (error != AL_NO_ERROR)
					return Report(error, message);
				else return OS::Logger::LogLevel::LOG_DEBUG;
			}

			namespace {
				static std::mutex OpenAL_API_Lock;
			}

			std::mutex& OpenALInstance::APILock() { return OpenAL_API_Lock; }

			Event<float, ActionQueue<>&>& OpenALInstance::OnTick() { return m_onTick; }



			const std::string& OpenALInstance::OpenALPhysicalDevice::Name()const { return m_name; }

			bool OpenALInstance::OpenALPhysicalDevice::IsDefaultDevice()const { return m_index == m_instance->DefaultDeviceId(); }

			Reference<AudioDevice> OpenALInstance::OpenALPhysicalDevice::CreateLogicalDevice() {
				return Object::Instantiate<OpenALDevice>(m_instance, this);
			}

			AudioInstance* OpenALInstance::OpenALPhysicalDevice::APIInstance()const { return m_instance; }

			void OpenALInstance::OpenALPhysicalDevice::OnOutOfScope()const { m_owner = nullptr; }
		}
	}
}

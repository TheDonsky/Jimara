#pragma once
#include "../AudioInstance.h"
#include "OpenALIncludes.h"
#include "../../Core/Systems/Event.h"
#include <thread>


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALInstance : public virtual AudioInstance {
			public:
				OpenALInstance(OS::Logger* logger);

				virtual ~OpenALInstance();

				virtual size_t PhysicalDeviceCount()const override;

				virtual Reference<PhysicalAudioDevice> PhysicalDevice(size_t index)const override;

				virtual size_t DefaultDeviceId()const override;

				OS::Logger::LogLevel Report(ALenum error, const char* message, OS::Logger::LogLevel minLevel = OS::Logger::LogLevel::LOG_DEBUG)const;

				OS::Logger::LogLevel ReportALCError(const char* message, OS::Logger::LogLevel minErrorLevel = OS::Logger::LogLevel::LOG_DEBUG)const;

				OS::Logger::LogLevel ReportALError(const char* message, OS::Logger::LogLevel minErrorLevel = OS::Logger::LogLevel::LOG_DEBUG)const;

				static std::mutex& APILock();

				Event<>& OnTick();


			private:
				class OpenALPhysicalDevice : public virtual PhysicalAudioDevice {
				public:
					virtual const std::string& Name()const override;

					virtual bool IsDefaultDevice()const override;

					virtual Reference<AudioDevice> CreateLogicalDevice() override;

					virtual AudioInstance* APIInstance()const override;

				protected:
					virtual void OnOutOfScope()const override;

				private:
					OpenALInstance* m_instance = nullptr;
					std::string m_name;
					size_t m_index = 0;
					mutable Reference<const Object> m_owner;

					friend class OpenALInstance;
				};

				OpenALPhysicalDevice* m_devices = nullptr;
				size_t m_deviceCount = 0;
				size_t m_defaultDeviceId = 0;

				std::thread m_tickThread;
				volatile bool m_killTick = false;
				EventInstance<> m_onTick;
			};
		}
	}
}

#pragma once
#include "../OS/Logging/Logger.h"
namespace Jimara { namespace Audio { class AudioInstance; } }
#include "PhysicalAudioDevice.h"


namespace Jimara {
	namespace Audio {
		class AudioInstance : public virtual Object {
		public:
			enum class Backend : uint8_t {
				OPEN_AL = 0,

				BACKEND_COUNT = 1
			};

			static Reference<AudioInstance> Create(OS::Logger* logger, Backend backend = Backend::OPEN_AL);

			virtual size_t PhysicalDeviceCount()const = 0;

			virtual Reference<PhysicalAudioDevice> PhysicalDevice(size_t index)const = 0;

			virtual size_t DefaultDeviceId()const = 0;

			inline Reference<PhysicalAudioDevice> DefaultDevice()const { return PhysicalDevice(DefaultDeviceId()); }

			inline OS::Logger* Log()const { return m_logger; }

		protected:
			inline AudioInstance(OS::Logger* logger) : m_logger(logger) {}

		private:
			const Reference<OS::Logger> m_logger;
		};
	}
}



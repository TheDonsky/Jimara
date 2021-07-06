#pragma once
#include "../OS/Logging/Logger.h"
namespace Jimara { namespace Audio { class AudioInstance; } }
#include "PhysicalAudioDevice.h"


namespace Jimara {
	namespace Audio {
		/// <summary>
		/// Audio framework/API instance for backend abstraction
		/// </summary>
		class AudioInstance : public virtual Object {
		public:
			/// <summary>
			/// Built-in audio backend types
			/// </summary>
			enum class Backend : uint8_t {
				/// <summary> OpenAL (implemented to function the best with openal-soft implementation) </summary>
				OPEN_AL = 0,

				/// <summary> Number of available built-in backends </summary>
				BACKEND_COUNT = 1
			};

			/// <summary>
			/// Instantiates a framework/API instance
			/// </summary>
			/// <param name="logger"> Logger to use for error reporting </param>
			/// <param name="backend"> Audio backend API </param>
			/// <returns> A new instance of AudioInstance </returns>
			static Reference<AudioInstance> Create(OS::Logger* logger, Backend backend = Backend::OPEN_AL);

			/// <summary> Number of audio devices, available to the system </summary>
			virtual size_t PhysicalDeviceCount()const = 0;

			/// <summary>
			/// Audio device by index
			/// </summary>
			/// <param name="index"> Audio device index (valid range: [0, PhysicalDeviceCount())) </param>
			/// <returns> Reference to the audio device by index </returns>
			virtual Reference<PhysicalAudioDevice> PhysicalDevice(size_t index)const = 0;

			/// <summary> Index of the system-wide default device </summary>
			virtual size_t DefaultDeviceId()const = 0;

			/// <summary> Reference to the system-wide default physical device </summary>
			inline Reference<PhysicalAudioDevice> DefaultDevice()const { return PhysicalDevice(DefaultDeviceId()); }

			/// <summary> Logger </summary>
			inline OS::Logger* Log()const { return m_logger; }

		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="logger"> Logger </param>
			inline AudioInstance(OS::Logger* logger) : m_logger(logger) {}

		private:
			// Logger
			const Reference<OS::Logger> m_logger;
		};
	}
}

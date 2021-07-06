#pragma once
#include "../AudioInstance.h"
#include "OpenALIncludes.h"
#include "../../Core/Systems/Event.h"
#include "../../Core/Systems/ActionQueue.h"
#include "../../Core/Stopwatch.h"
#include <thread>


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			/// <summary>
			/// OpenAL(-soft) based AudioInstance
			/// </summary>
			class OpenALInstance : public virtual AudioInstance {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="logger"> Logger for error reporting </param>
				OpenALInstance(OS::Logger* logger);

				/// <summary> Virtual destructor </summary>
				virtual ~OpenALInstance();

				/// <summary> Number of audio devices, available to the system </summary>
				virtual size_t PhysicalDeviceCount()const override;

				/// <summary>
				/// Audio device by index
				/// </summary>
				/// <param name="index"> Audio device index (valid range: [0, PhysicalDeviceCount())) </param>
				/// <returns> Reference to the audio device by index </returns>
				virtual Reference<PhysicalAudioDevice> PhysicalDevice(size_t index)const override;

				/// <summary> Index of the system-wide default device </summary>
				virtual size_t DefaultDeviceId()const override;

				/// <summary>
				/// Logs al/alc error
				/// </summary>
				/// <param name="error"> al/alc error </param>
				/// <param name="message"> error message/info </param>
				/// <returns> LogLevel from al/alc error </returns>
				OS::Logger::LogLevel Report(ALenum error, const char* message)const;

				/// <summary>
				/// Logs al/alc error if alcGetError(nullptr) returns any other value but AL_NO_ERROR
				/// </summary>
				/// <param name="message"> error message/info </param>
				/// <returns> LogLevel from al/alc error </returns>
				OS::Logger::LogLevel ReportALCError(const char* message)const;

				/// <summary>
				/// Logs al/alc error if alGetError() returns any other value but AL_NO_ERROR
				/// </summary>
				/// <param name="message"> error message/info </param>
				/// <returns> LogLevel from al/alc error </returns>
				OS::Logger::LogLevel ReportALError(const char* message)const;

				/// <summary> Reference to the global API-wide lock </summary>
				static std::mutex& APILock();

				/// <summary>
				/// Invoked on regular intervals to aid with internal state refreshes
				/// Note: params are delta time and ActionQueue<> reference, that will be flushed right after the tick.
				/// </summary>
				Event<float, ActionQueue<>&>& OnTick();


			private:
				/// <summary>
				/// OpenAL-backed physical device
				/// </summary>
				class OpenALPhysicalDevice : public virtual PhysicalAudioDevice {
				public:
					/// <summary> Devcie name </summary>
					virtual const std::string& Name()const override;

					/// <summary> True, if this device is selected as the system-wide default device </summary>
					virtual bool IsDefaultDevice()const override;

					/// <summary> Creates an instance of a logical AudioDevice </summary>
					virtual Reference<AudioDevice> CreateLogicalDevice() override;

					/// <summary> "Owner" AudioInstance </summary>
					virtual AudioInstance* APIInstance()const override;

				protected:
					/// <summary> Makes sure, there stay no circular references </summary>
					virtual void OnOutOfScope()const override;

				private:
					// "Owner" AudioInstance
					OpenALInstance* m_instance = nullptr;

					// Device name
					std::string m_name;

					// Device index
					size_t m_index = 0;

					// When reference count is non-zero, "Owner" AudioInstance will be stored here
					mutable Reference<const Object> m_owner;

					// OpenALInstance is allowed to alter the internals
					friend class OpenALInstance;
				};

				// Physical devices
				OpenALPhysicalDevice* m_devices = nullptr;

				// Number of physical devices
				size_t m_deviceCount = 0;

				// Default device index
				size_t m_defaultDeviceId = 0;

				// Thread, invoking OnTick()
				std::thread m_tickThread;

				// This one signals m_tickThread to die
				volatile bool m_killTick = false;

				// OnTick() event instance
				EventInstance<float, ActionQueue<>&> m_onTick;
			};
		}
	}
}

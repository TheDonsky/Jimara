#pragma once
namespace Jimara { namespace Audio { namespace OpenAL { class OpenALDevice; } } }
#include "OpenALInstance.h"
#include "OpenALContext.h"



namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			/// <summary>
			/// OpenAL-backed physical audio device
			/// </summary>
			class OpenALDevice : public virtual AudioDevice {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="instance"> OpenALInstance </param>
				/// <param name="physicalDevice"> Physical device to base the logical device on </param>
				OpenALDevice(OpenALInstance* instance, PhysicalAudioDevice* physicalDevice);

				/// <summary> Virtual destructor </summary>
				virtual ~OpenALDevice();

				/// <summary> Instantiates a new AudioScene to play around in </summary>
				virtual Reference<AudioScene> CreateScene() override;

				/// <summary>
				/// Creates a new Audio clip based on a buffer
				/// </summary>
				/// <param name="buffer"> Buffer to base the clip on </param>
				/// <param name="streamed"> If true, the AudioClip will not keep the whole buffer in memory all the time and will synamically load chunks as needed </param>
				/// <returns> A new instance of an AudioClip </returns>
				virtual Reference<AudioClip> CreateAudioClip(const AudioBuffer* buffer, bool streamed) override;

				/// <summary> Audio framework instance (pre-casted to OpenALInstance*) </summary>
				OpenALInstance* ALInstance()const;

				/// <summary> Underlying AL device </summary>
				operator ALCdevice* ()const;

				/// <summary> Default context for enabling resource creation without any listener </summary>
				OpenALContext* DefaultContext()const;

				/// <summary> Maximal number of sources that can be instantiated or played per listener </summary>
				size_t MaxSources()const;

			private:
				// Underlying AL device
				ALCdevice* m_device = nullptr;

				// Default context for enabling resource creation without any listener
				Reference<OpenALContext> m_defaultContext;

				// Maximal number of sources that can be instantiated or played per listener
				size_t m_maxSources = 0;
			};
		}
	}
}

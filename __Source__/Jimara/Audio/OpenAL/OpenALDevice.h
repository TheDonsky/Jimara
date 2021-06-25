#pragma once
namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALDevice;
		}
	}
}
#include "OpenALInstance.h"
#include "OpenALListener.h"



namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALDevice : public virtual AudioDevice {
			public:
				OpenALDevice(OpenALInstance* instance, PhysicalAudioDevice* physicalDevice);

				virtual ~OpenALDevice();

				virtual Reference<AudioScene> CreateScene() override;

				virtual Reference<AudioClip> CreateAudioClip(const AudioBuffer* buffer) override;

				OpenALInstance* ALInstance()const;

				operator ALCdevice* ()const;

				OpenALContext* DefaultContext()const;

			private:
				ALCdevice* m_device = nullptr;
				Reference<OpenALContext> m_defaultContext;
			};
		}
	}
}

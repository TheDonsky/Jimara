#pragma once
#include "OpenALInstance.h"



namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALDevice : public virtual AudioDevice {
			public:
				OpenALDevice(OpenALInstance* instance, PhysicalAudioDevice* physicalDevice);

				virtual ~OpenALDevice();

				virtual Reference<AudioScene> CreateScene() override;

				OpenALInstance* ALInstance()const;

				operator ALCdevice* ()const;

			private:
				ALCdevice* m_device = nullptr;
			};
		}
	}
}

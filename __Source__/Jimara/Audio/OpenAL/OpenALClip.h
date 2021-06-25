#pragma once
#include "OpenALDevice.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALClip : public virtual AudioClip {
			public:
				OpenALClip(OpenALDevice* device, const AudioBuffer* buffer);

				virtual ~OpenALClip();

				ALuint Buffer()const;

			private:
				const Reference<OpenALDevice> m_device;
				ALuint m_buffer = 0;
				bool m_bufferPresent = false;
			};
		}
	}
}

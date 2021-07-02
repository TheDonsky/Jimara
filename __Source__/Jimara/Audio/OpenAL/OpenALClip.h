#pragma once
namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALClip;
			class ClipPlayback;
		}
	}
}
#include "OpenALDevice.h"
#include "OpenALSource.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALClip : public virtual AudioClip {
			public:
				OpenALClip(OpenALDevice* device, const AudioBuffer* buffer);

				virtual ~OpenALClip();

				ALuint Buffer()const;

				virtual Reference<ClipPlayback> Play(ListenerContext* context, SourcePlayback* playback) { return nullptr; }

			private:
				const Reference<OpenALDevice> m_device;
				ALuint m_buffer = 0;
				bool m_bufferPresent = false;
			};

			class ClipPlayback : public virtual Object {

			};
		}
	}
}

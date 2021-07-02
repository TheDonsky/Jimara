#pragma once
namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALClip;
		}
	}
}
#include "OpenALDevice.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALClip : public virtual AudioClip {
			public:
				OpenALClip(OpenALDevice* device, const AudioBuffer* buffer);

				virtual ~OpenALClip();

				ALuint Buffer()const;

				class Player : public virtual Object {
				public:
					virtual AudioSource::PlaybackState State()const = 0;

					virtual void Play() = 0;

					virtual void Pause() = 0;

					virtual void Stop() = 0;

					virtual float Time()const = 0;

					virtual float SetTime(float time) = 0;
				};

			private:
				const Reference<OpenALDevice> m_device;
				ALuint m_buffer = 0;
				bool m_bufferPresent = false;
			};
		}
	}
}

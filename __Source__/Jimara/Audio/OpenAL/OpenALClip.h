#pragma once
namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALClip;
			class ClipPlayback;
			class ClipPlayback2D;
			class ClipPlayback3D;
		}
	}
}
#include "OpenALDevice.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALClip : public virtual AudioClip {
			public:
				static Reference<OpenALClip> Create(OpenALDevice* device, const AudioBuffer* buffer, bool streamed);

				OpenALDevice* Device()const;

				virtual Reference<ClipPlayback2D> Play2D(ListenerContext* context, const AudioSource2D::Settings& settings, bool loop, float timeOffset) = 0;

				virtual Reference<ClipPlayback3D> Play3D(ListenerContext* context, const AudioSource3D::Settings& settings, bool loop, float timeOffset) = 0;

			protected:
				OpenALClip(OpenALDevice* device, const AudioBuffer* buffer);

			private:
				const Reference<OpenALDevice> m_device;
			};

			class ClipPlayback : public virtual Object {
			public:
				ClipPlayback(ListenerContext* context);

				virtual ~ClipPlayback();

				virtual bool Playing() = 0;

			protected:
				ListenerContext* Context()const;

				ALuint Source()const;

			private:
				const Reference<ListenerContext> m_context;
				const ALuint m_source;
			};

			class ClipPlayback2D : public ClipPlayback {
			public:
				ClipPlayback2D(ListenerContext* context, const AudioSource2D::Settings& settings);

				void Update(const AudioSource2D::Settings& settings);
			};

			class ClipPlayback3D : public ClipPlayback {
			public:
				ClipPlayback3D(ListenerContext* context, const AudioSource3D::Settings& settings);

				void Update(const AudioSource3D::Settings& settings);
			};
		}
	}
}

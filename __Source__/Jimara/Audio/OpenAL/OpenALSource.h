#pragma once
#include "../../Core/Object.h"
namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALSource;
			class SourcePlayback;
			class SourcePlayback2D;
			class SourcePlayback3D;
		}
	}
}
#include "OpenALClip.h"
#include "OpenALScene.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALSource : public virtual AudioScene {
			public:

			};

			class SourcePlayback : public virtual Object {
			public:
				SourcePlayback(OpenALClip* clip) : m_clip(clip) {}

				OpenALClip* Clip()const { return m_clip; }

			private:
				Reference<OpenALClip> m_clip;
			};

			class SourcePlayback2D : public virtual SourcePlayback {
			public:

			};

			class SourcePlayback3D : public virtual SourcePlayback {
			public:

			};
		}
	}
}


#pragma once
#include "../../Core/Object.h"
namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALSource;

			class SourcePlayback : public virtual Object {
			public:
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
#include "OpenALScene.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALSource : public virtual AudioScene {
			public:

			};
		}
	}
}


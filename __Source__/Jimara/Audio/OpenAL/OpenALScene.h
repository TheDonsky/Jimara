#pragma once
#include "OpenALDevice.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALScene : public virtual AudioScene {
			public:
				OpenALScene(OpenALDevice* device);

				virtual ~OpenALScene();
			};
		}
	}
}

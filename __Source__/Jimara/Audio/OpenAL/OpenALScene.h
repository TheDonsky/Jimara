#pragma once
#include "OpenALDevice.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALScene : public virtual AudioScene {
			public:
				OpenALScene(OpenALDevice* device);

				virtual ~OpenALScene();

				virtual Reference<AudioSource2D> CreateSource2D(const AudioSource2D::Settings& settings, AudioClip* clip, bool play = true) override;

				virtual Reference<AudioSource3D> CreateSource3D(const AudioSource3D::Settings& settings, AudioClip* clip, bool play = true) override;

				virtual Reference<AudioListener> CreateListener(const AudioListener::Settings& settings) override;
			};
		}
	}
}

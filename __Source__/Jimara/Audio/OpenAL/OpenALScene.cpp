#include "OpenALScene.h"
#include "OpenALListener.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			OpenALScene::OpenALScene(OpenALDevice* device) : AudioScene(device) {}

			OpenALScene::~OpenALScene() {}

			Reference<AudioSource2D> OpenALScene::CreateSource2D(const AudioSource2D::Settings& settings, AudioClip* clip, bool play) {
				Device()->APIInstance()->Log()->Error("OpenALScene::CreateSource2D - Not yet implemented!");
				return nullptr;
			}

			Reference<AudioSource3D> OpenALScene::CreateSource3D(const AudioSource3D::Settings& settings, AudioClip* clip, bool play) {
				Device()->APIInstance()->Log()->Error("OpenALScene::CreateSource3D - Not yet implemented!");
				return nullptr;
			}

			Reference<AudioListener> OpenALScene::CreateListener(const AudioListener::Settings& settings) {
				return Object::Instantiate<OpenALListener>(settings, this);
			}
		}
	}
}

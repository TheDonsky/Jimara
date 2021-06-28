#pragma once
namespace Jimara { namespace Audio { class AudioScene; } }
#include "AudioDevice.h"
#include "AudioSource.h"
#include "AudioListener.h"



namespace Jimara {
	namespace Audio {
		class AudioScene : public virtual Object {
		public:
			virtual Reference<AudioSource2D> CreateSource2D(const AudioSource2D::Settings& settings, AudioClip* clip, bool play = true) = 0;

			virtual Reference<AudioSource3D> CreateSource3D(const AudioSource3D::Settings& settings, AudioClip* clip, bool play = true) = 0;

			virtual Reference<AudioListener> CreateListener(const AudioListener::Settings& settings) = 0;

			inline AudioDevice* Device()const { return m_device; }

		protected:
			inline AudioScene(AudioDevice* device) : m_device(device) {}

		private:
			const Reference<AudioDevice> m_device;
		};
	}
}


#pragma once
namespace Jimara { namespace Audio { class AudioScene; } }
#include "AudioDevice.h"



namespace Jimara {
	namespace Audio {
		class AudioScene : public virtual Object {
		public:
			inline AudioDevice* Device()const { return m_device; }

		protected:
			inline AudioScene(AudioDevice* device) : m_device(device) {}

		private:
			const Reference<AudioDevice> m_device;
		};
	}
}


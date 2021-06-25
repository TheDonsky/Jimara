#pragma once
namespace Jimara { namespace Audio { class AudioDevice; } }
#include "AudioInstance.h"
#include "AudioScene.h"


namespace Jimara {
	namespace Audio {
		class AudioDevice : public virtual Object {
		public:
			virtual Reference<AudioScene> CreateScene() = 0;

			virtual Reference<AudioClip> CreateAudioClip(const AudioBuffer* buffer) = 0;

			inline AudioInstance* APIInstance()const { return m_instance; }

			inline PhysicalAudioDevice* PhysicalDevice()const { return m_physicalDevice; }

		protected:
			inline AudioDevice(AudioInstance* instance, PhysicalAudioDevice* physicalDevice) : m_instance(instance), m_physicalDevice(physicalDevice) {}

		private:
			const Reference<AudioInstance> m_instance;
			const Reference<PhysicalAudioDevice> m_physicalDevice;
		};
	}
}
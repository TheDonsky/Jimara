#include "../GtestHeaders.h"
#include "../Memory.h"
#include "Audio/AudioInstance.h"
#include "OS/Logging/StreamLogger.h"



namespace Jimara {
	namespace Audio {
		TEST(AudioPlayground, Playground) {
			Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
			
			Reference<AudioInstance> instance = AudioInstance::Create(logger);
			ASSERT_NE(instance, nullptr);

			for (size_t i = 0; i < instance->PhysicalDeviceCount(); i++) {
				Reference<PhysicalAudioDevice> physicalDevice = instance->PhysicalDevice(i);
				ASSERT_NE(physicalDevice, nullptr);
				logger->Info(i, ". Name: <", physicalDevice->Name(), "> is default: ", physicalDevice->IsDefaultDevice() ? "true" : "false");
			}

			Reference<AudioDevice> device = instance->DefaultDevice()->CreateLogicalDevice();
			ASSERT_NE(device, nullptr);
		}
	}
}

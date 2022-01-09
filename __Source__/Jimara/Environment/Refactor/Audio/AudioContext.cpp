#include "AudioContext.h"

namespace Jimara {
#ifndef USE_REFACTORED_SCENE
namespace Refactor_TMP_Namespace {
#endif
	Reference<Scene::AudioContext> Scene::AudioContext::Create(Audio::AudioDevice* device, OS::Logger* logger) {
		Reference<Audio::AudioDevice> audioDevice = device;
		if (audioDevice == nullptr) {
			Reference<Audio::AudioInstance> instance = Audio::AudioInstance::Create(logger);
			if (instance == nullptr) {
				logger->Error("Scene::AudioContext::Create - Failed to create an AudioInstance!");
				return nullptr;
			}
			Reference<Audio::PhysicalAudioDevice> defaultDevice = instance->DefaultDevice();
			if (defaultDevice != nullptr)
				audioDevice = defaultDevice->CreateLogicalDevice();
			if (audioDevice == nullptr) {
				logger->Warning("Scene::AudioContext::Create - Failed to create the default audio device!");
				for (size_t i = 0; i < instance->PhysicalDeviceCount(); i++) {
					Reference<Audio::PhysicalAudioDevice> physicalDevice = instance->PhysicalDevice(i);
					if (physicalDevice == nullptr) continue;
					audioDevice = physicalDevice->CreateLogicalDevice();
					if (audioDevice != nullptr) break;
				}
				if (audioDevice == nullptr) {
					logger->Error("Scene::AudioContext::Create - Failed to create any AudioDevice!");
					return nullptr;
				}
			}
		}
		Reference<Audio::AudioScene> scene = audioDevice->CreateScene();
		if (scene == nullptr) {
			logger->Error("Scene::AudioContext::Create - Failed to create AudioScene!");
			return nullptr;
		}
		Reference<AudioContext> context = new AudioContext(scene);
		context->ReleaseRef();
		return context;
	}
#ifndef USE_REFACTORED_SCENE
}
#endif
}

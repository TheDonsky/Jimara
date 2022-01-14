#include "AudioContext.h"

namespace Jimara {
	Reference<Scene::AudioContext> Scene::AudioContext::Create(CreateArgs& createArgs) {
		if (createArgs.audio.audioDevice == nullptr) {
			if (createArgs.createMode == CreateArgs::CreateMode::CREATE_DEFAULT_FIELDS_AND_WARN)
				createArgs.logic.logger->Warning("Scene::AudioContext::Create - Audio device not provided! Creating a default device...");
			else if (createArgs.createMode == CreateArgs::CreateMode::ERROR_ON_MISSING_FIELDS) {
				createArgs.logic.logger->Error("Scene::AudioContext::Create - Audio device not provided!");
				return nullptr;
			}
			Reference<Audio::AudioInstance> instance = Audio::AudioInstance::Create(createArgs.logic.logger);
			if (instance == nullptr) {
				createArgs.logic.logger->Error("Scene::AudioContext::Create - Failed to create an AudioInstance!");
				return nullptr;
			}
			Reference<Audio::PhysicalAudioDevice> defaultDevice = instance->DefaultDevice();
			if (defaultDevice != nullptr)
				createArgs.audio.audioDevice = defaultDevice->CreateLogicalDevice();
			if (createArgs.audio.audioDevice == nullptr) {
				createArgs.logic.logger->Warning("Scene::AudioContext::Create - Failed to create the default audio device!");
				for (size_t i = 0; i < instance->PhysicalDeviceCount(); i++) {
					Reference<Audio::PhysicalAudioDevice> physicalDevice = instance->PhysicalDevice(i);
					if (physicalDevice == nullptr) continue;
					createArgs.audio.audioDevice = physicalDevice->CreateLogicalDevice();
					if (createArgs.audio.audioDevice != nullptr) break;
				}
				if (createArgs.audio.audioDevice == nullptr) {
					createArgs.logic.logger->Error("Scene::AudioContext::Create - Failed to create any AudioDevice!");
					return nullptr;
				}
			}
		}
		Reference<Audio::AudioScene> scene = createArgs.audio.audioDevice->CreateScene();
		if (scene == nullptr) {
			createArgs.logic.logger->Error("Scene::AudioContext::Create - Failed to create AudioScene!");
			return nullptr;
		}
		Reference<AudioContext> context = new AudioContext(scene);
		context->ReleaseRef();
		return context;
	}
}

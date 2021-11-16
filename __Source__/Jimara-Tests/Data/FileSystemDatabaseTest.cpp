#pragma once
#include "../GtestHeaders.h"
#include "../CountingLogger.h"
#include "Data/AssetDatabase/FileSystemDatabase/FileSystemDatabase.h"


namespace Jimara {
	TEST(FileSystemDatabaseTest, CreateDB) {
		Reference<Jimara::Test::CountingLogger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();

		Reference<Graphics::GraphicsDevice> graphicsDevice = [&]() -> Reference<Graphics::GraphicsDevice> {
			Reference<Application::AppInformation> appInformation =
				Object::Instantiate<Application::AppInformation>("FileSystemDatabaseTest", Application::AppVersion(0, 0, 1));
			Reference<Graphics::GraphicsInstance> graphicsInstance = Graphics::GraphicsInstance::Create(logger, appInformation);
			if (graphicsInstance == nullptr)
				return nullptr;
			for (size_t i = 0; i < graphicsInstance->PhysicalDeviceCount(); i++)
				if (graphicsInstance->GetPhysicalDevice(i)->Type() == Graphics::PhysicalDevice::DeviceType::DESCRETE) {
					Reference<Graphics::GraphicsDevice> device = graphicsInstance->GetPhysicalDevice(i)->CreateLogicalDevice();
					if (device != nullptr) return device;
				}
			for (size_t i = 0; i < graphicsInstance->PhysicalDeviceCount(); i++)
				if (graphicsInstance->GetPhysicalDevice(i)->Type() == Graphics::PhysicalDevice::DeviceType::INTEGRATED) {
					Reference<Graphics::GraphicsDevice> device = graphicsInstance->GetPhysicalDevice(i)->CreateLogicalDevice();
					if (device != nullptr) return device;
				}
			for (size_t i = 0; i < graphicsInstance->PhysicalDeviceCount(); i++) {
				Reference<Graphics::GraphicsDevice> device = graphicsInstance->GetPhysicalDevice(i)->CreateLogicalDevice();
				if (device != nullptr) return device;
			}
			return nullptr;
		}();
		ASSERT_NE(graphicsDevice, nullptr);

		Reference<Audio::AudioDevice> audioDevice = [&]() -> Reference<Audio::AudioDevice> {
			Reference<Audio::AudioInstance> audioInstance = Audio::AudioInstance::Create(logger);
			if (audioInstance == nullptr) return nullptr;
			if (audioInstance->DefaultDevice() != nullptr) {
				Reference<Audio::AudioDevice> device = audioInstance->DefaultDevice()->CreateLogicalDevice();
				if (device != nullptr) return device;
			}
			for (size_t i = 0; i < audioInstance->PhysicalDeviceCount(); i++) {
				Reference<Audio::AudioDevice> device = audioInstance->PhysicalDevice(i)->CreateLogicalDevice();
				if (device != nullptr) return device;
			}
			return nullptr;
		}();
		ASSERT_NE(audioDevice, nullptr);

		Reference<FileSystemDatabase> database = Object::Instantiate<FileSystemDatabase>(graphicsDevice, audioDevice);
		ASSERT_NE(database, nullptr);

		database->RescanAll();
	}
}

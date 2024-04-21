#pragma once
#include "Graphics/GraphicsDevice.h"
#include "OS/Logging/StreamLogger.h"


namespace Jimara {
	namespace Test {
		inline static Reference<Graphics::GraphicsDevice> CreateTestGraphicsDevice() {
			const Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
			const Reference<Application::AppInformation> appInfo = Object::Instantiate<Application::AppInformation>("Jimara_BitonicSortTest");
			const Reference<Graphics::GraphicsInstance> instance = Graphics::GraphicsInstance::Create(logger, appInfo);
			if (instance == nullptr) {
				logger->Error("Jimara::Test::CreateTestGraphicsDevice - Failed to create graphics instance!");
				return nullptr;
			}
			Graphics::PhysicalDevice* physicalDevice = nullptr;
			for (size_t i = 0; i < instance->PhysicalDeviceCount(); i++) {
				Graphics::PhysicalDevice* device = instance->GetPhysicalDevice(i);
				if (device == nullptr || (!device->HasFeatures(Graphics::PhysicalDevice::DeviceFeatures::COMPUTE)))
					continue;
				if (physicalDevice == nullptr)
					physicalDevice = device;
				else if (
					(device->Type() != Graphics::PhysicalDevice::DeviceType::VIRTUAL) &&
					(device->Type() > physicalDevice->Type() || physicalDevice->Type() == Graphics::PhysicalDevice::DeviceType::VIRTUAL))
					physicalDevice = device;
			}
			if (physicalDevice == nullptr) {
				logger->Error("Jimara::Test::CreateTestGraphicsDevice - No compatible device found on the system!");
				return nullptr;
			}
			const Reference<Graphics::GraphicsDevice> device = physicalDevice->CreateLogicalDevice();
			if (device == nullptr)
				logger->Error("Jimara::Test::CreateTestGraphicsDevice - Failed to create graphics device!");
			return device;
		}
	}
}

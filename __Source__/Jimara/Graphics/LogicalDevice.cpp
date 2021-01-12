#include "LogicalDevice.h"


namespace Jimara {
	namespace Graphics {
		LogicalDevice::~LogicalDevice() {}

		PhysicalDevice* LogicalDevice::PhysicalDevice()const { return m_physicalDevice; }

		GraphicsInstance* LogicalDevice::GraphicsInstance()const { return m_physicalDevice->GraphicsInstance(); }

		OS::Logger* LogicalDevice::Log()const { return m_physicalDevice->Log(); }

		LogicalDevice::LogicalDevice(Graphics::PhysicalDevice* physicalDevice) : m_physicalDevice(physicalDevice) {}
	}
}

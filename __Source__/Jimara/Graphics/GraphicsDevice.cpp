#include "GraphicsDevice.h"


namespace Jimara {
	namespace Graphics {
		GraphicsDevice::~GraphicsDevice() {}

		PhysicalDevice* GraphicsDevice::PhysicalDevice()const { return m_physicalDevice; }

		GraphicsInstance* GraphicsDevice::GraphicsInstance()const { return m_physicalDevice->GraphicsInstance(); }

		OS::Logger* GraphicsDevice::Log()const { return m_physicalDevice->Log(); }

		GraphicsDevice::GraphicsDevice(Graphics::PhysicalDevice* physicalDevice) : m_physicalDevice(physicalDevice) {}
	}
}

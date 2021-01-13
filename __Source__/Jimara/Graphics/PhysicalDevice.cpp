#include "PhysicalDevice.h"

namespace Jimara {
	namespace Graphics {
		PhysicalDevice::~PhysicalDevice() {}

		void PhysicalDevice::AddRef()const { m_owner->AddRef(); }

		void PhysicalDevice::ReleaseRef()const { m_owner->ReleaseRef(); }

		GraphicsInstance* PhysicalDevice::GraphicsInstance()const { return m_owner; }

		OS::Logger* PhysicalDevice::Log()const { return m_owner->Log(); }

		bool PhysicalDevice::HasFeature(DeviceFeature feature)const { return (Features() & static_cast<uint64_t>(feature)) != 0; }

		PhysicalDevice::PhysicalDevice(Graphics::GraphicsInstance* instance) : m_owner(instance) {}
	}
}

#include "PhysicalDevice.h"

namespace Jimara {
	namespace Graphics {
		PhysicalDevice::~PhysicalDevice() {}

		void PhysicalDevice::AddRef()const { m_owner->AddRef(); }

		void PhysicalDevice::ReleaseRef()const { m_owner->ReleaseRef(); }

		GraphicsInstance* PhysicalDevice::GraphicsInstance()const { return m_owner; }

		OS::Logger* PhysicalDevice::Log()const { return m_owner->Log(); }

		bool PhysicalDevice::HasFeatures(DeviceFeatures features)const { return (Features() & features) == features; }

		PhysicalDevice::PhysicalDevice(Graphics::GraphicsInstance* instance) : m_owner(instance) {}

		std::ostream& operator<<(std::ostream& stream, PhysicalDevice::DeviceFeatures features) {
			using VType = std::underlying_type_t<PhysicalDevice::DeviceFeatures>;
			const constexpr uint32_t BIT_COUNT = sizeof(VType) * 8u;
			static_assert(std::is_same_v<VType, uint64_t>);
			auto forAllfeatures = [](auto action) {
				action(PhysicalDevice::DeviceFeatures::GRAPHICS, "GRAPHICS");
				action(PhysicalDevice::DeviceFeatures::COMPUTE, "COMPUTE");
				action(PhysicalDevice::DeviceFeatures::SYNCHRONOUS_COMPUTE, "SYNCH_COMPUTE");
				action(PhysicalDevice::DeviceFeatures::ASYNCHRONOUS_COMPUTE, "ASYNCH_COMPUTE");
				action(PhysicalDevice::DeviceFeatures::SWAP_CHAIN, "SWAP_CHAIN");
				action(PhysicalDevice::DeviceFeatures::SAMPLER_ANISOTROPY, "SAMPLER_ANISOTROPY");
				action(PhysicalDevice::DeviceFeatures::FRAGMENT_SHADER_INTERLOCK, "FRAGMENT_INTERLOCK");
				action(PhysicalDevice::DeviceFeatures::RAY_TRACING, "RAY_TRACING");
			};

			uint32_t setBitCount = 0u;
			PhysicalDevice::DeviceFeatures unknownBits = features;
			forAllfeatures([&](PhysicalDevice::DeviceFeatures f, const char*) {
				if ((f & features) == PhysicalDevice::DeviceFeatures::NONE)
					return;
				setBitCount++;
				unknownBits ^= f;
				assert((unknownBits & f) == PhysicalDevice::DeviceFeatures::NONE);
				});

			const bool displayAsList = (setBitCount != 1u || unknownBits != PhysicalDevice::DeviceFeatures::NONE);
			if (displayAsList)
				stream << '(';
			uint32_t tokenCount = 0u;
			auto printToken = [&](const auto& token) {
				if (tokenCount <= 0u)
					stream << ' ';
				stream << token;
				tokenCount++;
			};
			forAllfeatures([&](PhysicalDevice::DeviceFeatures f, const char* name) {
				if ((f & features) != PhysicalDevice::DeviceFeatures::NONE)
					printToken(name);
				});
			if (unknownBits != PhysicalDevice::DeviceFeatures::NONE)
				printToken(static_cast<VType>(unknownBits));
			if (displayAsList)
				stream << ')';
			return stream;
		}
	}
}

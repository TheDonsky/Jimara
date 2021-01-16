#include "RenderSurface.h"

namespace Jimara {
	namespace Graphics {
		GraphicsInstance* RenderSurface::GraphicsInstance()const { return m_graphicsInstance; }

		PhysicalDevice* RenderSurface::PrefferedDevice()const {
			bool(*deviceViable)(const PhysicalDevice*, const RenderSurface*) = [](const PhysicalDevice* info, const RenderSurface* surface) {
				return (info != nullptr)
					&& ((info->Features() & static_cast<uint64_t>(Graphics::PhysicalDevice::DeviceFeature::GRAPHICS)) != 0)
					&& ((info->Features() & static_cast<uint64_t>(Graphics::PhysicalDevice::DeviceFeature::COMPUTE)) != 0)
					&& ((info->Features() & static_cast<uint64_t>(Graphics::PhysicalDevice::DeviceFeature::SWAP_CHAIN)) != 0)
					&& ((info->Features() & static_cast<uint64_t>(Graphics::PhysicalDevice::DeviceFeature::SAMPLER_ANISOTROPY)) != 0)
					&& surface->DeviceCompatible(info);
			};

			const size_t deviceCount = GraphicsInstance()->PhysicalDeviceCount();
			size_t bestId = deviceCount;
			uint8_t bestIsDescrete = 0;
			uint64_t bestVRAM = 0;
			for (size_t deviceId = 0; deviceId < deviceCount; deviceId++) {
				const Graphics::PhysicalDevice* info = GraphicsInstance()->GetPhysicalDevice(deviceId);
				if (!deviceViable(info, this)) continue;
				
				uint8_t isDescrete = ((info->Type() == Graphics::PhysicalDevice::DeviceType::DESCRETE) ? 1 : 0);
				if (deviceId > 0) {
					if (bestIsDescrete > isDescrete) continue;
					else if ((bestIsDescrete == isDescrete) && bestVRAM >= info->VramCapacity()) continue;
				}
				bestId = deviceId;
				bestIsDescrete = isDescrete;
				bestVRAM = info->VramCapacity();
			}

			return (bestId >= deviceCount) ? nullptr : GraphicsInstance()->GetPhysicalDevice(bestId);
		}

		RenderSurface::RenderSurface(Graphics::GraphicsInstance* graphicsInstance) : m_graphicsInstance(graphicsInstance) {}
	}
}

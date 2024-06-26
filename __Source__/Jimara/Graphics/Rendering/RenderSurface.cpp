#include "RenderSurface.h"

namespace Jimara {
	namespace Graphics {
		GraphicsInstance* RenderSurface::GraphicsInstance()const { return m_graphicsInstance; }

		PhysicalDevice* RenderSurface::PrefferedDevice()const {
			bool(*deviceViable)(const PhysicalDevice*, const RenderSurface*) = [](const PhysicalDevice* info, const RenderSurface* surface) {
				return (info != nullptr) && info->HasFeatures(
					Graphics::PhysicalDevice::DeviceFeatures::GRAPHICS |
					Graphics::PhysicalDevice::DeviceFeatures::COMPUTE |
					Graphics::PhysicalDevice::DeviceFeatures::SWAP_CHAIN |
					Graphics::PhysicalDevice::DeviceFeatures::SAMPLER_ANISOTROPY)
					&& surface->DeviceCompatible(info);
			};

			struct DeviceRank {
				size_t deviceId = ~size_t(0u);
				size_t typeRank = 0u;
				size_t featureRank = 0u;
				size_t vramCapacity = 0u;

				inline bool operator<(const DeviceRank& other) const {
					return 
						(typeRank < other.typeRank) ? true : (typeRank > other.typeRank) ? false :
						(featureRank < other.featureRank) ? true : (featureRank > other.featureRank) ? false :
						(vramCapacity < other.vramCapacity);
				}
			};

			const size_t deviceCount = GraphicsInstance()->PhysicalDeviceCount();
			DeviceRank bestRank;
			for (size_t deviceId = 0; deviceId < deviceCount; deviceId++) {
				const Graphics::PhysicalDevice* info = GraphicsInstance()->GetPhysicalDevice(deviceId);
				if (!deviceViable(info, this)) continue;
				
				DeviceRank rank = {};
				{
					rank.deviceId = deviceId;
					switch (info->Type()) {
					case Graphics::PhysicalDevice::DeviceType::OTHER:
						rank.typeRank = 2u;
						break;
					case Graphics::PhysicalDevice::DeviceType::DESCRETE:
						rank.typeRank = 32u;
						break;
					case Graphics::PhysicalDevice::DeviceType::INTEGRATED:
						rank.typeRank = 16u;
						break;
					case Graphics::PhysicalDevice::DeviceType::VIRTUAL:
						rank.typeRank = 8u;
						break;
					case Graphics::PhysicalDevice::DeviceType::CPU:
						rank.typeRank = 1u;
						break;
					default:
						break;
					}
					rank.featureRank |=
						(info->HasFeatures(PhysicalDevice::DeviceFeatures::FRAGMENT_SHADER_INTERLOCK) ? 1u : 0u) | 
						(info->HasFeatures(PhysicalDevice::DeviceFeatures::RAY_TRACING) ? 128u : 0u);
					rank.vramCapacity = info->VramCapacity();
				}

				if (bestRank.deviceId >= deviceCount || bestRank < rank)
					bestRank = rank;
			}

			return (bestRank.deviceId >= deviceCount) ? nullptr : GraphicsInstance()->GetPhysicalDevice(bestRank.deviceId);
		}

		RenderSurface::RenderSurface(Graphics::GraphicsInstance* graphicsInstance) : m_graphicsInstance(graphicsInstance) {}
	}
}

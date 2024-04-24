#pragma once
#include "Buffers/VulkanArrayBuffer.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
#pragma warning(disable: 4250)
			/// <summary>
			/// Scratch buffer;
			/// <para/> For internal use only; Will not be supported via general API...
			/// </summary>
			class VulkanScratchBufferProvider : public virtual ObjectCache<Reference<const Object>>::StoredObject {
			public:
				/// <summary>
				/// Returns shared instance of the scratch buffer
				/// </summary>
				/// <param name="device"> Vulkan device </param>
				/// <returns> Scratch buffer instance </returns>
				inline static Reference<VulkanScratchBufferProvider> Get(VulkanDevice* device) {
					if (device == nullptr)
						return nullptr;
					struct Cache : public virtual ObjectCache<Reference<const Object>> {
						inline Reference<VulkanScratchBufferProvider> GetFor(VulkanDevice* device) {
							return GetCachedOrCreate(device, [&]() {
								const Reference<VulkanScratchBufferProvider> rv = new VulkanScratchBufferProvider(device);
								rv->ReleaseRef();
								return rv;
								});
						}
					};
					static Cache cache;
					return cache.GetFor(device);
				}

				/// <summary> Virtual destructor </summary>
				inline virtual ~VulkanScratchBufferProvider() {}

				/// <summary>
				/// Gets buffer based on the minimal size
				/// </summary>
				/// <param name="minSize"> Minimal size </param>
				/// <returns> Buffer </returns>
				inline Reference<VulkanArrayBuffer> GetBuffer(size_t minSize) {
					std::unique_lock<SpinLock> lock(m_bufferLock);
					Reference<VulkanArrayBuffer> result = m_buffer;
					if (result != nullptr && result->ObjectCount() >= minSize)
						return result;
					const size_t size = Math::Max(
						(result == nullptr) ? 1u : (result->ObjectCount() << 1u), minSize);
					result = Object::Instantiate<VulkanArrayBuffer>(
						m_device, 1u, size, true,
						VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
						VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
						VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
					if (result != nullptr)
						m_buffer = result;
					return result;
				}

			private:
				SpinLock m_bufferLock;
				Reference<VulkanArrayBuffer> m_buffer;
				const Reference<VulkanDevice> m_device;

				inline VulkanScratchBufferProvider(VulkanDevice* device) : m_device(device) { assert(m_device != nullptr); }
			};
#pragma warning(default: 4250)
		}
	}
}


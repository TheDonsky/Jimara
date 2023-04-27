#pragma once
#include "../VulkanDevice.h"
#include "../Memory/Textures/VulkanImage.h"
#include "../Pipeline/Commands/VulkanCommandPool.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Vulkan-backed render engine.
			/// Supports arbitrary renderers that output to images.
			/// </summary>
			class JIMARA_API VulkanRenderEngine : public RenderEngine {
			public:
				/// <summary> "Owner" Vulkan device </summary>
				inline VulkanDevice* Device()const { return m_device; }

			protected:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> "Owner" vulkan device </param>
				inline VulkanRenderEngine(VulkanDevice* device) : m_device(device) {}

			private:
				// "Owner" vulkan device
				Reference<VulkanDevice> m_device;
			};
		}
	}
}

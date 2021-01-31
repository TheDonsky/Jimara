#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanImageView;
			class VulkanStaticImageView;
		}
	}
}
#include "../Textures/VulkanTexture.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Basic interface for Vulkan image views
			/// </summary>
			class VulkanImageView : public virtual TextureView {
			public:
				/// <summary>
				/// Access immutable handle to VkImageView
				/// </summary>
				/// <param name="commandRecorder"> Command recorder for flushing any modifications if necessary </param>
				/// <returns> Reference to the view </returns>
				virtual Reference<VulkanStaticImageView> GetStaticHandle(VulkanCommandRecorder* commandRecorder) = 0;
			};
			
			/// <summary>
			/// Basic interface for direct wrapper on top of VkImageView objects
			/// </summary>
			class VulkanStaticImageView : public virtual VulkanImageView {
			public:
				/// <summary> Type cast to API object </summary>
				virtual operator VkImageView()const = 0;

				/// <summary>
				/// Access immutable handle to VkImageView
				/// </summary>
				/// <returns> Reference to the texture </returns>
				inline virtual Reference<VulkanStaticImageView> GetStaticHandle(VulkanCommandRecorder*) override { return this; }
			};
		}
	}
}

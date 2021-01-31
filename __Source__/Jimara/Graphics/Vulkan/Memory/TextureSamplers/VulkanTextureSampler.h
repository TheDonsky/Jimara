#pragma once
#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanImageSampler;
			class VulkanStaticImageSampler;
		}
	}
}
#include "../TextureViews/VulkanTextureView.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Basic interface for Vulkan image samplers
			/// </summary>
			class VulkanImageSampler : public virtual TextureSampler {
			public:
				/// <summary>
				/// Access immutable handle to VkSampler
				/// </summary>
				/// <param name="commandRecorder"> Command recorder for flushing any modifications if necessary </param>
				/// <returns> Reference to the sampler </returns>
				virtual Reference<VulkanStaticImageSampler> GetStaticHandle(VulkanCommandRecorder* commandRecorder) = 0;
			};

			/// <summary>
			/// Basic interface for direct wrapper on top of VkSampler objects
			/// </summary>
			class VulkanStaticImageSampler : public virtual VulkanImageSampler {
			public:
				/// <summary> Type cast to API object </summary>
				virtual operator VkSampler()const = 0;

				/// <summary>
				/// Access immutable handle to VkImageView
				/// </summary>
				/// <returns> Reference to the sampler </returns>
				inline virtual Reference<VulkanStaticImageSampler> GetStaticHandle(VulkanCommandRecorder*) override { return this; }
			};
		}
	}
}

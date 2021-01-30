#pragma once
#include "VulkanTexture.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			//TextureType type
			//	, uint32_t baseMipLevel = 0, uint32_t mipLevelCount = ~((uint32_t)0u)
			//	, uint32_t baseArrayLayer = 0, uint32_t arrayLayerCount = ~((uint32_t)0u);

			/// <summary>
			/// Wrapper on top of a VkImageView object
			/// </summary>
			class VulkanImageView : public virtual TextureView {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="image"> Image </param>
				/// <param name="viewType"> View type </param>
				/// <param name="baseMipLevel"> Base mip level </param>
				/// <param name="mipLevelCount"> Number of view mip levels </param>
				/// <param name="baseArrayLayer"> Base array slice </param>
				/// <param name="arrayLayerCount"> Number of view array slices </param>
				/// <param name="aspectFlags"> Aspect flgs </param>
				VulkanImageView(VulkanImage* image, ViewType viewType
					, uint32_t baseMipLevel, uint32_t mipLevelCount
					, uint32_t baseArrayLayer, uint32_t arrayLayerCount
					, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanImageView();

				/// <summary> Target image </summary>
				VulkanImage* Image()const;

				/// <summary> Type cast to API object </summary>
				operator VkImageView()const;

				/// <summary> Type of the view </summary>
				virtual ViewType Type()const override;

				/// <summary> Texture, this view belongs to </summary>
				virtual Texture* TargetTexture()const override;

				/// <summary>
				/// Creates an image sampler
				/// </summary>
				/// <param name="filtering"> Image filtering mode </param>
				/// <param name="wrapping"> Tells, how the image outside the bounds is sampled </param>
				/// <param name="lodBias"> Lod bias </param>
				/// <returns> New instance of a texture sampler </returns>
				virtual Reference<TextureSampler> CreateSampler(
					TextureSampler::FilteringMode filtering = TextureSampler::FilteringMode::LINEAR
					, TextureSampler::WrappingMode wrapping = TextureSampler::WrappingMode::REPEAT
					, float lodBias = 0)override;



			private:
				// Target image
				const Reference<VulkanImage> m_image;

				// Image view type
				const ViewType m_viewType;

				// Aspect flags
				const VkImageAspectFlags m_aspectFlags;

				// Underlying API object
				VkImageView m_view;
			};
		}
	}
}

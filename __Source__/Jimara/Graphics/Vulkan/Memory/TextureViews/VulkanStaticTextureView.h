#pragma once
#include "VulkanTextureView.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Wrapper on top of a VkImageView object
			/// </summary>
			class VulkanStaticTextureView : public virtual VulkanStaticImageView {
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
				VulkanStaticTextureView(VulkanStaticImage* image, ViewType viewType
					, uint32_t baseMipLevel, uint32_t mipLevelCount
					, uint32_t baseArrayLayer, uint32_t arrayLayerCount);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanStaticTextureView();

				/// <summary> Type cast to API object </summary>
				virtual operator VkImageView()const override;

				/// <summary> Type of the view </summary>
				virtual ViewType Type()const override;

				/// <summary> Texture, this view belongs to </summary>
				virtual Texture* TargetTexture()const override;

				/// <summary> Base mip level </summary>
				virtual uint32_t BaseMipLevel()const override;

				/// <summary> Number of view mip levels </summary>
				virtual uint32_t MipLevelCount()const override;

				/// <summary> Base array slice </summary>
				virtual uint32_t BaseArrayLayer()const override;

				/// <summary> Number of view array slices </summary>
				virtual uint32_t ArrayLayerCount()const override;

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
				const Reference<VulkanStaticImage> m_image;

				// Image view type
				const ViewType m_viewType;

				// Base mip level
				const uint32_t m_baseMipLevel;
				
				// Number of view mip levels
				const uint32_t m_mipLevelCount;
				
				// Base array layer
				const uint32_t m_baseArrayLayer;

				// Number of view array layers
				const uint32_t m_arrayLayerCount;

				// Underlying API object
				VkImageView m_view;
			};
		}
	}
}

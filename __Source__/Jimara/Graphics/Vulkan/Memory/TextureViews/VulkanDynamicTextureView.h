#pragma once
#include "VulkanTextureView.h"
#include "../../../../Core/Synch/SpinLock.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// View to an arbitrary vulkan image
			/// </summary>
			class VulkanDynamicTextureView : public virtual VulkanImageView {
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
				VulkanDynamicTextureView(VulkanImage* image, ViewType viewType
					, uint32_t baseMipLevel, uint32_t mipLevelCount
					, uint32_t baseArrayLayer, uint32_t arrayLayerCount);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanDynamicTextureView();

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
				/// Access immutable handle to VkImageView
				/// </summary>
				/// <param name="commandBuffer"> Command buffer thar may rely on the resource </param>
				/// <returns> Reference to the view </returns>
				virtual Reference<VulkanStaticImageView> GetStaticHandle(VulkanCommandBuffer* commandBuffer) override;

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

				// Base mip level
				const uint32_t m_baseMipLevel;

				// Number of view mip levels
				const uint32_t m_mipLevelCount;

				// Base array layer
				const uint32_t m_baseArrayLayer;

				// Number of view array layers
				const uint32_t m_arrayLayerCount;

				// Spinlock for protecting m_view in particular
				SpinLock m_viewSpin;

				// Underlying view
				Reference<VulkanStaticImageView> m_view;

				// View reference protection
				std::mutex m_viewLock;
			};
		}
	}
}

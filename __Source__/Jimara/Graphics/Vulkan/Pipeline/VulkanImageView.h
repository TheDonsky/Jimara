#pragma once
#include "../Memory/VulkanTexture.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Wrapper on top of a VkImageView object
			/// </summary>
			class VulkanImageView : public virtual Object {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="image"> Image </param>
				/// <param name="viewType"> View type </param>
				/// <param name="aspectFlags"> Aspect flags </param>
				VulkanImageView(VulkanImage* image, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanImageView();

				/// <summary> Target image </summary>
				VulkanImage* Image()const;

				/// <summary> Type cast to API object </summary>
				operator VkImageView()const;



			private:
				// Target image
				Reference<VulkanImage> m_image;

				// Image view type
				VkImageViewType m_viewType;

				// Aspect flags
				VkImageAspectFlags m_aspectFlags;

				// Underlying API object
				VkImageView m_view;
			};
		}
	}
}

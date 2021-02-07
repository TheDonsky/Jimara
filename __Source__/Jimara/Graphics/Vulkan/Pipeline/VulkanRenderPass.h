#pragma once
#include "../Memory/Textures/VulkanTexture.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Simple wrapper on top of a VkRenderPass object with a single subpass
			/// </summary>
			class VulkanRenderPass : public virtual Object {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> "Owner" device </param>
				/// <param name="sampleCount"> "MSAA" </param>
				/// <param name="numColorAttachments"> Color attachment count </param>
				/// <param name="colorAttachmentFormats"> Pixel format per color attachment </param>
				/// <param name="depthFormat"> Depth format (if value is outside [FIRST_DEPTH_FORMAT; LAST_DEPTH_FORMAT] range, the render pass will not have a depth format) </param>
				/// <param name="includeResolveAttachments"> If true, the render pass will include a resolve attachment for each of the multisampled color attachment </param>
				VulkanRenderPass(VulkanDevice* device, Texture::Multisampling sampleCount
					, size_t numColorAttachments, Texture::PixelFormat* colorAttachmentFormats
					, Texture::PixelFormat depthFormat, bool includeResolveAttachments);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanRenderPass();

				/// <summary> Type cast to API object </summary>
				operator VkRenderPass()const;

				/// <summary> "Owner" device </summary>
				VulkanDevice* Device()const;

				/// <summary> MSAA </summary>
				Texture::Multisampling Multisampling()const;

				/// <summary> Number of color attachments </summary>
				size_t ColorAttachmentCount()const;

				/// <summary>
				/// Piexel format for a color attachment with given id (is same as resolve attachment format with given index)
				/// </summary>
				/// <param name="imageId"> Color attachment index </param>
				/// <returns> Pixel format of the color attachment </returns>
				Texture::PixelFormat ColorAttachmentFormat(size_t imageId)const;

				/// <summary> First color attachment index witin the framebuffer layout </summary>
				size_t FirstColorAttachmentId()const;

				/// <summary> True, if there is supposed to be a depth attachment </summary>
				bool HasDepthAttachment()const;

				/// <summary> Depth attachment format </summary>
				Texture::PixelFormat DepthAttachmentFormat()const;

				/// <summary> Index of the depth attachment witin the framebuffer layout </summary>
				size_t DepthAttachmentId()const;

				/// <summary> True, if there are supposed to be resolve attachments corresponding to color attachments </summary>
				bool HasResolveAttachments()const;

				/// <summary> Index of the first resolve attachment within the framebuffer layout </summary>
				size_t FirstResolveAttachmentId()const;


			private:
				// "Owner" device
				const Reference<VulkanDevice> m_device;

				// MSAA
				const Texture::Multisampling m_sampleCount;

				// Color attachment formats
				const std::vector<Texture::PixelFormat> m_colorAttachmentFormats;

				// Depth attachment format
				const Texture::PixelFormat m_depthAttachmentFormat;

				// True, if resolve attachments are present
				const bool m_hasResolveAttachments;

				// Underlying API object
				VkRenderPass m_renderPass;
			};
		}
	}
}

#pragma once
#include "../Memory/TextureViews/VulkanTextureView.h"
#include <vector>

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary> Wrapper on top of a VkFramebuffer object </summary>
			class VulkanFrameBuffer : public virtual Object {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="attachments"> Attachment views </param>
				/// <param name="renderPass"> Compatible render pass (not really needed post-creation, so no Jimara object here) </param>
				VulkanFrameBuffer(const std::vector<Reference<VulkanStaticImageView>>& attachments, VkRenderPass renderPass);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanFrameBuffer();

				/// <summary> Type cast to the underlying API object </summary>
				operator VkFramebuffer()const;


			private:
				// Attachments
				const std::vector<Reference<VulkanStaticImageView>> m_attachments;
				
				// Underlying API object
				VkFramebuffer m_frameBuffer;
			};
		}
	}
}

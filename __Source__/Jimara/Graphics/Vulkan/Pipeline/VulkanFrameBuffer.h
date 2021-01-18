#pragma once
#include "../Pipeline/VulkanImageView.h"
#include <vector>

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanFrameBuffer : public virtual Object {
			public:
				VulkanFrameBuffer(const std::vector<Reference<VulkanImageView>>& attachments);

				virtual ~VulkanFrameBuffer();

				operator VkFramebuffer()const;


			private:
				std::vector<Reference<VulkanImageView>> m_attachments;
				VkFramebuffer m_frameBuffer;
			};
		}
	}
}

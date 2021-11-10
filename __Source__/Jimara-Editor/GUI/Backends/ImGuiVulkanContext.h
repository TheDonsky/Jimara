#pragma once
#include "../ImGuiDeviceContext.h"
#include <Graphics/Vulkan/VulkanDevice.h>
#include <Graphics/Vulkan/Pipeline/VulkanRenderPass.h>


namespace Jimara {
	namespace Editor {
		class ImGuiVulkanContext : public virtual ImGuiDeviceContext {
		public:
			ImGuiVulkanContext(Graphics::Vulkan::VulkanDevice* device);

			~ImGuiVulkanContext();

			virtual Reference<ImGuiWindowContext> GetWindowContext(OS::Window* window) override;

			virtual Reference<ImGuiRenderer> CreateRenderer(ImGuiWindowContext* windowContext, const Graphics::RenderEngineInfo* renderEngineInfo) override;

			Graphics::RenderPass* RenderPass()const;

			static Graphics::Texture::PixelFormat FrameFormat();

			void SetImageCount(size_t imageCount);

		private:
			const Reference<Graphics::Vulkan::VulkanRenderPass> m_renderPass;
			VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
			bool m_vulkanContextInitialized = false;
			uint32_t m_imageCount = 4;
		};
	}
}

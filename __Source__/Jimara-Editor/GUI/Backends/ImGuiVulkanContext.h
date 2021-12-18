#pragma once
#include "ImGuiDeviceContext.h"
#include "ImGuiWindowContext.h"
#include <Graphics/Vulkan/VulkanDevice.h>
#include <Graphics/Vulkan/Pipeline/VulkanRenderPass.h>


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// ImGuiDeviceContext for Vulkan Graphics API
		/// </summary>
		class ImGuiVulkanContext : public virtual ImGuiDeviceContext {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="device"> Device, this context is tied to </param>
			/// <param name="frameFormat"> Pixel format for ImGui </param>
			ImGuiVulkanContext(Graphics::Vulkan::VulkanDevice* device, Graphics::Texture::PixelFormat frameFormat);

			/// <summary> Virtual Destructor </summary>
			~ImGuiVulkanContext();

			/// <summary>
			/// Creates ImGuiWindowContext instance for given window
			/// </summary>
			/// <param name="window"> Window to render to </param>
			/// <returns> Per-Window ImGui content </returns>
			static Reference<ImGuiWindowContext> CreateWindowContext(OS::Window* window);

			/// <summary> Render pass for ImGui </summary>
			Graphics::RenderPass* RenderPass()const;

			/// <summary>
			/// Updates max in-flight image count if it's less than what is required
			/// Notes: Used only by ImGuiVulkanRenderer; This one is sort of unsafe for the current ImGui implementation
			/// </summary>
			/// <param name="imageCount"> Max in-flight image count necessary for the renderer to function </param>
			void SetImageCount(size_t imageCount);

		private:
			// Render pass for ImGui
			const Reference<Graphics::Vulkan::VulkanRenderPass> m_renderPass;

			// Descriptor pool
			VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

			// True, if ImGui_ImplVulkan_Init did not fail (failure would result in a Fatal error, so probably always true if you have an instance)
			bool m_vulkanContextInitialized = false;

			// Max in-flight image count (we picked the large-ish number so that SetImageCount() is always a no-op while the ImGui Vulkan issue persists)
			uint32_t m_imageCount = 5;
		};
	}
}

#pragma once
#include "../ImGuiDeviceContext.h"
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
			ImGuiVulkanContext(Graphics::Vulkan::VulkanDevice* device);

			/// <summary> Virtual Destructor </summary>
			~ImGuiVulkanContext();

			/// <summary>
			/// Gets ImGuiWindowContext instance for given window
			/// Note: There will only be a single instance of an ImGuiWindowContext per window, this function will not create multiple ones.
			/// </summary>
			/// <param name="window"> Window to render to </param>
			/// <returns> Per-Window ImGui content </returns>
			virtual Reference<ImGuiWindowContext> GetWindowContext(OS::Window* window) override;

			/// <summary>
			/// Creates a new instance of an ImGuiRenderer tied to the given ImGuiWindowContext and the RenderEngineInfo(from some generic render engine)
			/// Notes: 
			///		0. Render engine should likely be from a surface from the same window and the same Graphics device for predictable behaviour;
			///		1. One would expect to create ImGuiRenderer objects as a part of a per-RenderEngine data for an ImageRenderer.
			/// </summary>
			/// <param name="windowContext"> Window Context </param>
			/// <param name="renderEngineInfo"> Surface render engine info </param>
			/// <returns> New instance of an ImGui renderer </returns>
			virtual Reference<ImGuiRenderer> CreateRenderer(ImGuiWindowContext* windowContext, const Graphics::RenderEngineInfo* renderEngineInfo) override;

			/// <summary> Render pass for ImGui </summary>
			Graphics::RenderPass* RenderPass()const;

			/// <summary> Pixel format for ImGui </summary>
			static Graphics::Texture::PixelFormat FrameFormat();

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

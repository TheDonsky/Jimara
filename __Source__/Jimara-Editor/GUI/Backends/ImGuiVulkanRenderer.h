#pragma once
#include "ImGuiVulkanContext.h"


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// ImGuiRenderer for Vulkan backend
		/// </summary>
		class ImGuiVulkanRenderer : public virtual ImGuiRenderer {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="guiContext"> Per-Device ImGui context </param>
			/// <param name="windowContext"> Per-Window ImGui context </param>
			/// <param name="renderEngineInfo"> Render engine information </param>
			ImGuiVulkanRenderer(ImGuiVulkanContext* guiContext, ImGuiWindowContext* windowContext, const Graphics::RenderEngineInfo* renderEngineInfo);

			/// <summary> Virtual destructor </summary>
			virtual ~ImGuiVulkanRenderer();

		protected:
#ifdef JIMARA_EDITOR_ImGuiRenderer_RenderFrameAtomic
			/// <summary>
			/// Begins frame, executes given action and ends frame atomically
			/// </summary>
			/// <param name="execute"> Job to execute between starting and ending the frame </param>
			virtual void RenderFrame(Callback<> execute) override;

#else
			/// <summary> Begins ImGui frame </summary>
			virtual void BeginFrame() override;

			/// <summary> Ends ImGui frame </summary>
			virtual void EndFrame() override;
#endif

			/// <summary>
			/// Creates ImGui-drawable texture from Engine texture
			/// </summary>
			/// <param name="sampler"> Texture sampler </param>
			/// <returns> Gui-drawable texture </returns>
			virtual Reference<ImGuiTexture> CreateTexture(Graphics::TextureSampler* sampler) override;

		private:
			// Per-Device ImGui context
			Reference<ImGuiVulkanContext> m_deviceContext;

			// Per-Window ImGui context
			Reference<ImGuiWindowContext> m_windowContext;

			// Render engine information
			const Reference<const Graphics::RenderEngineInfo> m_engineInfo;

			// Frame buffer and the correponding target view
			const std::pair<Reference<Graphics::TextureView>, Reference<Graphics::FrameBuffer>> m_frameBuffer;

			// Texture data
			class ImGuiVulkanRendererTexture;
		};
	}
}

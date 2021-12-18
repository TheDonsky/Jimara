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
			/// <summary> Begins ImGui frame </summary>
			virtual void BeginFrame() override;

			/// <summary> Ends ImGui frame </summary>
			virtual void EndFrame() override;

			/// <summary>
			/// Draws a texture
			/// </summary>
			/// <param name="texture"> Texture to draw </param>
			/// <param name="rect"> Rect to draw at </param>
			virtual void DrawTexture(Graphics::Texture* texture, const Rect& rect) override;

		private:
			// Per-Device ImGui context
			Reference<ImGuiVulkanContext> m_deviceContext;

			// Per-Window ImGui context
			Reference<ImGuiWindowContext> m_windowContext;

			// Render engine information
			const Reference<const Graphics::RenderEngineInfo> m_engineInfo;

			// Frame buffer and the correponding target view
			const std::pair<Reference<Graphics::TextureView>, Reference<Graphics::FrameBuffer>> m_frameBuffer;

			// DrawTexture commands, recorded for the later use
			std::vector<std::pair<Reference<Graphics::Texture>, std::pair<SizeRect, SizeRect>>> m_drawTextureCommands;

			// Index of the last executed DrawTexture from m_drawTextureCommands
			volatile size_t m_drawTextureId = 0;
		};
	}
}

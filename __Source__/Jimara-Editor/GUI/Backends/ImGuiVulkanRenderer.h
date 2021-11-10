#pragma once
#include "ImGuiVulkanContext.h"


namespace Jimara {
	namespace Editor {
		class ImGuiVulkanRenderer : public virtual ImGuiRenderer {
		public:
			ImGuiVulkanRenderer(ImGuiVulkanContext* guiContext, ImGuiWindowContext* windowContext, const Graphics::RenderEngineInfo* renderEngineInfo);

			virtual ~ImGuiVulkanRenderer();

		protected:
			virtual void BeginFrame() override;

			virtual void EndFrame() override;

			virtual void DrawTexture(Graphics::Texture* texture, const Rect& rect) override;

		private:
			const Reference<ImGuiVulkanContext> m_deviceContext;
			const Reference<ImGuiWindowContext> m_windowContext;
			const Reference<const Graphics::RenderEngineInfo> m_engineInfo;
			const std::pair<Reference<Graphics::TextureView>, Reference<Graphics::FrameBuffer>> m_frameBuffer;

			std::vector<std::pair<Reference<Graphics::Texture>, std::pair<SizeRect, SizeRect>>> m_drawTextureCommands;
			volatile size_t m_drawTextureId = 0;
		};
	}
}

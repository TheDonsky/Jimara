#include "ImGuiVulkanRenderer.h"
#pragma warning(disable: 26812)
#include <backends/imgui_impl_vulkan.h>
#pragma warning(default: 26812)


namespace Jimara {
	namespace Editor {
		ImGuiVulkanRenderer::ImGuiVulkanRenderer(ImGuiVulkanContext* guiContext, ImGuiWindowContext* windowContext, const Graphics::RenderEngineInfo* renderEngineInfo) 
			: ImGuiRenderer(guiContext), m_deviceContext(guiContext), m_windowContext(windowContext), m_engineInfo(renderEngineInfo)
			, m_frameBuffer([&]()->std::pair<Reference<Graphics::TextureView>, Reference<Graphics::FrameBuffer>> {
			Reference<Graphics::Texture> texture = guiContext->GraphicsDevice()->CreateMultisampledTexture(
				Graphics::Texture::TextureType::TEXTURE_2D, renderEngineInfo->ImageFormat(), Size3(renderEngineInfo->ImageSize(), 1), 1, Graphics::Texture::Multisampling::SAMPLE_COUNT_1);
			Reference<Graphics::TextureView> view = texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
			return std::make_pair(view, guiContext->RenderPass()->CreateFrameBuffer(&view, nullptr, nullptr, nullptr));
				}()) {}

		ImGuiVulkanRenderer::~ImGuiVulkanRenderer() {
			m_deviceContext = nullptr;
			m_windowContext = nullptr;
		}


		void ImGuiVulkanRenderer::BeginFrame() {
			m_deviceContext->SetImageCount(m_engineInfo->ImageCount());
			m_windowContext->BeginFrame();
			ImGui_ImplVulkan_NewFrame();
			ImGui::NewFrame();
		}

		void ImGuiVulkanRenderer::EndFrame() {
			ImGui::Render();
			m_windowContext->EndFrame();
			ImDrawData* draw_data = ImGui::GetDrawData();
			if (draw_data->DisplaySize.x > 0.0f && draw_data->DisplaySize.y > 0.0f) {
				m_deviceContext->RenderPass()->BeginPass(ImGuiRenderer::BufferInfo().commandBuffer, m_frameBuffer.second, nullptr, false);
				ImGui_ImplVulkan_RenderDrawData(draw_data, *dynamic_cast<Graphics::Vulkan::VulkanCommandBuffer*>(ImGuiRenderer::BufferInfo().commandBuffer));
				m_deviceContext->RenderPass()->EndPass(ImGuiRenderer::BufferInfo().commandBuffer);
				m_engineInfo->Image(ImGuiRenderer::BufferInfo().inFlightBufferId)->Blit(ImGuiRenderer::BufferInfo().commandBuffer, m_frameBuffer.first->TargetTexture());
			}
			m_drawTextureId = 0;
			m_drawTextureCommands.clear();
		}

		void ImGuiVulkanRenderer::DrawTexture(Graphics::Texture* texture, const Rect& rect) {
			if (texture == nullptr) return;

			const bool flipX = (rect.start.x > rect.end.x);
			const bool flipY = (rect.start.y > rect.end.y);
			const Vector2 start(flipX ? rect.end.x : rect.start.x, flipY ? rect.end.y : rect.start.y);
			const Vector2 end(flipX ? rect.start.x : rect.end.x, flipY ? rect.start.y : rect.end.y);
			const Vector2 size = (end - start);

			if (end.x <= 0.0f || end.y <= 0.0f) return;
			else if (size.x <= 0.0f || size.y <= 0.0f) return;
			auto toVec2 = [](const ImVec2& v) { return Jimara::Vector2(v.x, v.y); };
			const Vector2 viewportSize = toVec2(ImGui::GetMainViewport()->Size);
			if (start.x >= viewportSize.x || start.y >= viewportSize.y) return;

			const Rect cutRect(
				Vector2(max(start.x, 0.0f), max(start.y, 0.0f)),
				Vector2(min(end.x, viewportSize.x), min(end.y, viewportSize.y)));
			if (cutRect.start.x >= cutRect.end.x || cutRect.start.y >= cutRect.end.y) return;

			const Vector2 imageSize = texture->Size();
			SizeRect imageRect(
				(Size2)(imageSize * (cutRect.start - start) / size),
				(Size2)(imageSize * (1.0f - (end - cutRect.end) / size)));
			if (flipX) std::swap(imageRect.start.x, imageRect.end.x);
			if (flipY) std::swap(imageRect.start.y, imageRect.end.y);

			m_drawTextureCommands.push_back(std::make_pair(texture, std::make_pair(SizeRect(cutRect.start, cutRect.end), imageRect)));
			ImGui::GetWindowDrawList()->AddCallback([](const ImDrawList* parent_list, const ImDrawCmd* cmd) {
				ImGuiVulkanRenderer* self = (ImGuiVulkanRenderer*)cmd->UserCallbackData;
				if (ImGuiRenderer::BufferInfo().commandBuffer == nullptr) return;
				self->m_deviceContext->RenderPass()->EndPass(ImGuiRenderer::BufferInfo().commandBuffer);
				std::pair<Reference<Graphics::Texture>, std::pair<SizeRect, SizeRect>> desc = self->m_drawTextureCommands[self->m_drawTextureId];
				self->m_frameBuffer.first->TargetTexture()->Blit(ImGuiRenderer::BufferInfo().commandBuffer, desc.first,
					SizeAABB(Size3(desc.second.first.start, 0), Size3(desc.second.first.end, 1)),
					SizeAABB(Size3(desc.second.second.start, 0), Size3(desc.second.second.end, 1)));
				self->m_drawTextureId++;
				self->m_deviceContext->RenderPass()->BeginPass(ImGuiRenderer::BufferInfo().commandBuffer, self->m_frameBuffer.second, nullptr, false);
				}, this);
			ImGui::GetWindowDrawList()->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
		}
	}
}

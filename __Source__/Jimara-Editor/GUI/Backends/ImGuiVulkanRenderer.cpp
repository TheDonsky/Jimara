#include "ImGuiVulkanRenderer.h"
#pragma warning(disable: 26812)
#include <backends/imgui_impl_vulkan.h>
#pragma warning(default: 26812)
#include <Jimara/Graphics/Vulkan/Memory/Textures/VulkanTextureSampler.h>


namespace Jimara {
	namespace Editor {
		ImGuiVulkanRenderer::ImGuiVulkanRenderer(ImGuiVulkanContext* guiContext, ImGuiWindowContext* windowContext, const Graphics::RenderEngineInfo* renderEngineInfo)
			: ImGuiRenderer(guiContext), m_deviceContext(guiContext), m_windowContext(windowContext), m_engineInfo(renderEngineInfo)
#ifndef JIMARA_EDITOR_ImGuiVulkanContext_PixlFormatOverride
			, m_frameBuffers([&]() -> std::vector<Reference<Graphics::FrameBuffer>> {
			std::vector<Reference<Graphics::FrameBuffer>> buffers;
			for (size_t i = 0u; i < renderEngineInfo->ImageCount(); i++) {
				Reference<Graphics::TextureView> view = renderEngineInfo->Image(i)->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
				buffers.push_back(guiContext->RenderPass()->CreateFrameBuffer(&view, nullptr, nullptr, nullptr));
			}
			return buffers;
				}()) {}
#else
			, m_frameBuffer([&]()->std::pair<Reference<Graphics::TextureView>, Reference<Graphics::FrameBuffer>> {
			Reference<Graphics::Texture> texture = guiContext->GraphicsDevice()->CreateMultisampledTexture(
				Graphics::Texture::TextureType::TEXTURE_2D, JIMARA_EDITOR_ImGuiVulkanContext_PixlFormatOverride, //renderEngineInfo->ImageFormat(),
				Size3(renderEngineInfo->ImageSize(), 1), 1, Graphics::Texture::Multisampling::SAMPLE_COUNT_1);
			Reference<Graphics::TextureView> view = texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
			return std::make_pair(view, guiContext->RenderPass()->CreateFrameBuffer(&view, nullptr, nullptr, nullptr));
				}()) {}
#endif

		ImGuiVulkanRenderer::~ImGuiVulkanRenderer() {
			m_deviceContext = nullptr;
			m_windowContext = nullptr;
		}


#ifdef JIMARA_EDITOR_ImGuiRenderer_RenderFrameAtomic
		void ImGuiVulkanRenderer::RenderFrame(Callback<> execute) {
			m_deviceContext->SetImageCount(m_engineInfo->ImageCount());
			auto call = [&]() {
				ImGui_ImplVulkan_NewFrame();
				ImGui::NewFrame();
				execute();
				ImGui::Render();
				ImDrawData* draw_data = ImGui::GetDrawData();
				if (draw_data->DisplaySize.x > 0.0f && draw_data->DisplaySize.y > 0.0f) {
					Graphics::FrameBuffer* frameBuffer = 
#ifndef JIMARA_EDITOR_ImGuiVulkanContext_PixlFormatOverride
						m_frameBuffers[ImGuiRenderer::BufferInfo().inFlightBufferId];
#else
						m_frameBuffer.second;
#endif
					m_deviceContext->RenderPass()->BeginPass(ImGuiRenderer::BufferInfo().commandBuffer, frameBuffer, nullptr, false);
					ImGui_ImplVulkan_RenderDrawData(draw_data, *dynamic_cast<Graphics::Vulkan::VulkanCommandBuffer*>(ImGuiRenderer::BufferInfo().commandBuffer));
					m_deviceContext->RenderPass()->EndPass(ImGuiRenderer::BufferInfo().commandBuffer);
#ifdef JIMARA_EDITOR_ImGuiVulkanContext_PixlFormatOverride
					//m_engineInfo->Image(ImGuiRenderer::BufferInfo().inFlightBufferId)->Blit(ImGuiRenderer::BufferInfo(), m_frameBuffer.first->TargetTexture());
					m_engineInfo->Image(ImGuiRenderer::BufferInfo().inFlightBufferId)->Copy(ImGuiRenderer::BufferInfo(), m_frameBuffer.first->TargetTexture());
#endif
				}
			};
			m_windowContext->RenderFrame(Callback<>::FromCall(&call));
		}

#else
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
		}
#endif

		class ImGuiVulkanRenderer::ImGuiVulkanRendererTexture : public virtual ImGuiTexture {
		public:
			inline ImGuiVulkanRendererTexture(ImGuiVulkanContext* context, Graphics::TextureSampler* sampler) 
				: m_context(context), m_sampler(sampler)
				, m_textureId(ImGui_ImplVulkan_AddTexture(
					*dynamic_cast<Graphics::Vulkan::VulkanTextureSampler*>(sampler),
					*dynamic_cast<Graphics::Vulkan::VulkanTextureView*>(sampler->TargetView()), VK_IMAGE_LAYOUT_GENERAL))
				, m_self(this) {}

			inline virtual ~ImGuiVulkanRendererTexture() {
				ImGuiAPIContext::Lock lock(m_context->APIContext());
				vkFreeDescriptorSets(*dynamic_cast<Graphics::Vulkan::VulkanDevice*>(m_context->GraphicsDevice()), m_context->DescriptorPool(), 1, &m_textureId);
			}

			virtual operator ImTextureID()const final override {
				ImGuiAPIContext::Lock lock(m_context->APIContext());
				Graphics::Vulkan::VulkanCommandBuffer* commandBuffer =
					dynamic_cast<Graphics::Vulkan::VulkanCommandBuffer*>(ImGuiRenderer::BufferInfo().commandBuffer);
				commandBuffer->RecordBufferDependency(m_self);
				return static_cast<ImTextureID>(m_textureId);
			}

		private:
			const Reference<ImGuiVulkanContext> m_context;
			const Reference<Graphics::TextureSampler> m_sampler;
			const VkDescriptorSet m_textureId;
			ImGuiVulkanRendererTexture* const m_self;
		};

		Reference<ImGuiTexture> ImGuiVulkanRenderer::CreateTexture(Graphics::TextureSampler* sampler) {
			return Object::Instantiate<ImGuiVulkanRendererTexture>(m_deviceContext, sampler);
		}
	}
}

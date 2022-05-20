#include "ImGuiVulkanRenderer.h"
#pragma warning(disable: 26812)
#include <backends/imgui_impl_vulkan.h>
#pragma warning(default: 26812)
#include <Graphics/Vulkan/Memory/TextureSamplers/VulkanTextureSampler.h>


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
		}

		class ImGuiVulkanRenderer::ImGuiVulkanRendererTexture : public virtual ImGuiTexture {
		public:
			inline ImGuiVulkanRendererTexture(
				ImGuiVulkanContext* context,
				Graphics::TextureSampler* sampler) : m_context(context), m_sampler(sampler) {}

			inline virtual ~ImGuiVulkanRendererTexture() {}

			virtual operator ImTextureID()const final override {
				Graphics::Vulkan::VulkanImageSampler* sampler = dynamic_cast<Graphics::Vulkan::VulkanImageSampler*>(m_sampler.operator->());
				if (sampler == nullptr) {
					m_context->APIContext()->Log()->Fatal("ImGuiVulkanRendererTexture::operator ImTextureID - Expected Vulkan sampler!");
					return 0;
				}

				ImGuiAPIContext::Lock lock(m_context->APIContext());
				Graphics::Vulkan::VulkanCommandBuffer* commandBuffer =
					dynamic_cast<Graphics::Vulkan::VulkanCommandBuffer*>(ImGuiRenderer::BufferInfo().commandBuffer);
				
				Reference<Graphics::Vulkan::VulkanStaticImageSampler> staticSampler = sampler->GetStaticHandle(commandBuffer);
				if (staticSampler == nullptr) {
					m_context->APIContext()->Log()->Fatal("ImGuiVulkanRendererTexture::operator ImTextureID - Failed to get static sampler!");
					return 0;
				}

				Graphics::Vulkan::VulkanStaticImageView* staticView = dynamic_cast<Graphics::Vulkan::VulkanStaticImageView*>(staticSampler->TargetView());
				if (staticView == nullptr) {
					m_context->APIContext()->Log()->Fatal("ImGuiVulkanRendererTexture::operator ImTextureID - Failed to read static view!");
					return 0;
				}

				if ((!m_textureId.has_value()) || m_staticSampler == nullptr) {
					m_textureId = ImGui_ImplVulkan_AddTexture(*staticSampler, *staticView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
					m_textureIdHolder = Object::Instantiate<DescriptorSetHolder>(m_context, m_textureId.value());
				}

				commandBuffer->RecordBufferDependency(m_textureIdHolder);
				commandBuffer->RecordBufferDependency(m_staticSampler);
				return static_cast<ImTextureID>(m_textureId.value());
			}

		private:
			const Reference<ImGuiVulkanContext> m_context;
			const Reference<Graphics::TextureSampler> m_sampler;
			Reference<Graphics::Vulkan::VulkanStaticImageSampler> m_staticSampler;
			mutable std::optional<VkDescriptorSet> m_textureId;
			
			class DescriptorSetHolder : public virtual Object {
			private:
				const Reference<ImGuiVulkanContext> m_context;
				VkDescriptorSet m_set;

			public:
				inline DescriptorSetHolder(ImGuiVulkanContext* context, VkDescriptorSet set)
					: m_context(context), m_set(set) {}

				inline virtual ~DescriptorSetHolder() {
					ImGuiAPIContext::Lock lock(m_context->APIContext());
					vkFreeDescriptorSets(*dynamic_cast<Graphics::Vulkan::VulkanDevice*>(m_context->GraphicsDevice()), m_context->DescriptorPool(), 1, &m_set);
				}
			};
			mutable Reference<DescriptorSetHolder> m_textureIdHolder;
		};

		Reference<ImGuiTexture> ImGuiVulkanRenderer::CreateTexture(Graphics::TextureSampler* sampler) {
			return Object::Instantiate<ImGuiVulkanRendererTexture>(m_deviceContext, sampler);
		}
	}
}

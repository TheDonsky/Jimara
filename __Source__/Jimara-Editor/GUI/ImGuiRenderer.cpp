#include "ImGuiRenderer.h"


namespace Jimara {
	namespace Editor {
		namespace {
			static Graphics::Pipeline::CommandBufferInfo g_bufferInfo;
			static std::atomic<ImGuiRenderer*> g_current = nullptr;
			static std::atomic<bool> g_fieldModifiedFlag = false;
		}

		void ImGuiRenderer::Render(const Graphics::Pipeline::CommandBufferInfo& bufferInfo) {
			auto cleanup = []() {
				g_fieldModifiedFlag = false;
				g_current = nullptr;
				g_bufferInfo = Graphics::Pipeline::CommandBufferInfo();
			};

			auto execute = [&]() {
				ImGuiAPIContext::Lock lock(m_deviceContext->APIContext());
				g_bufferInfo = bufferInfo;
				g_current = this;
				g_fieldModifiedFlag = false;
#ifndef JIMARA_EDITOR_ImGuiRenderer_RenderFrameAtomic
				BeginFrame();
#endif
				m_jobs.Execute();
#ifndef JIMARA_EDITOR_ImGuiRenderer_RenderFrameAtomic
				EndFrame();
				cleanup();
#endif
			};
#ifdef JIMARA_EDITOR_ImGuiRenderer_RenderFrameAtomic
			RenderFrame(Callback<>::FromCall(&execute));
			{
				ImGuiAPIContext::Lock lock(m_deviceContext->APIContext());
				cleanup();
			}
#else
			execute();
#endif
		}

		const Graphics::Pipeline::CommandBufferInfo& ImGuiRenderer::BufferInfo() {
			return g_bufferInfo;
		}

		Reference<ImGuiTexture> ImGuiRenderer::Texture(Graphics::TextureSampler* sampler) {
			if (g_current != nullptr) return g_current.load()->CreateTexture(sampler);
			else return nullptr;
		}

		void ImGuiRenderer::FieldModified() {
			g_fieldModifiedFlag = true;
		}

		bool ImGuiRenderer::AnyFieldModified() {
			return g_fieldModifiedFlag;
		}

		void ImGuiRenderer::AddRenderJob(JobSystem::Job* job) {
			m_jobs.Add(job);
		}

		void ImGuiRenderer::RemoveRenderJob(JobSystem::Job* job) {
			m_jobs.Remove(job);
		}
	}
}

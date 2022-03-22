#include "ImGuiRenderer.h"


namespace Jimara {
	namespace Editor {
		namespace {
			static Graphics::Pipeline::CommandBufferInfo g_bufferInfo;
			static std::atomic<ImGuiRenderer*> g_current = nullptr;
			static std::atomic<bool> g_fieldModifiedFlag = false;
		}

		void ImGuiRenderer::Render(const Graphics::Pipeline::CommandBufferInfo& bufferInfo) {
			ImGuiAPIContext::Lock lock(m_deviceContext->APIContext());
			g_bufferInfo = bufferInfo;
			g_current = this;
			g_fieldModifiedFlag = false;
			BeginFrame();
			m_jobs.Execute();
			EndFrame();
			g_fieldModifiedFlag = false;
			g_current = nullptr;
			g_bufferInfo = Graphics::Pipeline::CommandBufferInfo();
		}

		const Graphics::Pipeline::CommandBufferInfo& ImGuiRenderer::BufferInfo() {
			return g_bufferInfo;
		}

		void ImGuiRenderer::Texture(Graphics::Texture* texture, const Rect& rect) {
			if (g_current != nullptr) g_current.load()->DrawTexture(texture, rect);
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

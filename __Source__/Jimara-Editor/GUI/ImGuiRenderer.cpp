#include "ImGuiRenderer.h"


namespace Jimara {
	namespace Editor {
		namespace {
			static Graphics::Pipeline::CommandBufferInfo g_bufferInfo;
			static std::atomic<ImGuiRenderer*> g_current = nullptr;
		}

		void ImGuiRenderer::Render(const Graphics::Pipeline::CommandBufferInfo& bufferInfo) {
			ImGuiAPIContext::Lock lock(m_deviceContext->APIContext());
			g_bufferInfo = bufferInfo;
			g_current = this;
			BeginFrame();
			m_jobs.Execute();
			EndFrame();
			g_current = nullptr;
			g_bufferInfo = Graphics::Pipeline::CommandBufferInfo();
		}

		const Graphics::Pipeline::CommandBufferInfo& ImGuiRenderer::BufferInfo() {
			return g_bufferInfo;
		}

		void ImGuiRenderer::Texture(Graphics::Texture* texture, const Rect& rect) {
			if (g_current != nullptr) g_current.load()->DrawTexture(texture, rect);
		}

		bool ImGuiRenderer::MenuAction(const std::string_view& menuPath, const void* actionId) {
			static thread_local std::vector<char> nameBuffer;
			size_t menus = 0;
			bool rv = false;
			for (size_t i = 0; i < menuPath.size(); i++) {
				const char symbol = menuPath[i];
				if (i >= (menuPath.size() - 1)) {
					nameBuffer.push_back(symbol);
					{
						static const char* ACTION_ID_PRE = "###MenuAction_";
						const char* ptr = ACTION_ID_PRE;
						while (true) {
							const char c = *ptr;
							if (c == 0) break;
							else {
								nameBuffer.push_back(c);
								ptr++;
							}
						}
					}
					{
						size_t ptrToI = ((size_t)actionId);
						for (size_t p = 0; p < (sizeof(size_t) * 2); p++) {
							nameBuffer.push_back('a' + (char)(ptrToI & 15));
							ptrToI >>= 4;
						}
					}
					nameBuffer.push_back(0);
					rv = ImGui::MenuItem(nameBuffer.data());
					break;
				}
				else if (symbol == '/' || symbol == '\\') {
					if (nameBuffer.size() > 0) {
						nameBuffer.push_back(0);
						if (!ImGui::BeginMenu(nameBuffer.data())) break;
						else {
							nameBuffer.clear();
							menus++;
						}
					}
				}
				else nameBuffer.push_back(symbol);
			}
			nameBuffer.clear();
			for (size_t i = 0; i < menus; i++)
				ImGui::EndMenu();
			return rv;
		}

		void ImGuiRenderer::AddRenderJob(JobSystem::Job* job) {
			m_jobs.Add(job);
		}

		void ImGuiRenderer::RemoveRenderJob(JobSystem::Job* job) {
			m_jobs.Remove(job);
		}
	}
}

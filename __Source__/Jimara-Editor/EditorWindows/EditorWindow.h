#pragma once
#include "../Environment/JimaraEditor.h"
#include "../GUI/ImGuiRenderer.h"

namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Basic template for an editor window
		/// </summary>
		class EditorWindow : public virtual Object {
		public:
			/// <summary> Editor context </summary>
			inline EditorContext* EditorWindowContext()const { return m_context; }

			/// <summary> Window name/header </summary>
			std::string& EditorWindowName() { return m_name; }

			/// <summary> Window name/header </summary>
			const std::string& EditorWindowName()const { return m_name; }

			/// <summary> Closes the window </summary>
			void Close() { m_open = false; }

		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Editor context </param>
			/// <param name="name"> Editor window name </param>
			inline EditorWindow(EditorContext* context, const std::string_view& name)
				: m_context(context), m_name(name) {
				Object::Instantiate<WindowDisplayJob>(this);
			}

			/// <summary>
			/// Should draw the contents of the editor window
			/// </summary>
			virtual void DrawEditorWindow() {}

		private:
			// Job, that invokes window runtime callbacks
			class WindowDisplayJob : public JobSystem::Job {
			private:
				const Reference<EditorWindow> m_window;
			public:
				inline WindowDisplayJob(EditorWindow* window) : m_window(window) {
					m_window->EditorWindowContext()->AddRenderJob(this);
				}
				inline virtual ~WindowDisplayJob() {}
				virtual void Execute() final override {
					bool open = m_window->m_open.load();
					if (open) {
						std::string name = [&]() -> std::string {
							std::stringstream stream;
							stream << m_window->EditorWindowName() << "###EditorWindow_" << ((size_t)m_window.operator->());
							return stream.str();
						}();
						if (ImGui::Begin(name.c_str(), &open)) {
							ImGui::SetWindowSize(ImVec2(384.0f, 0.0f), ImGuiCond_FirstUseEver);
							m_window->DrawEditorWindow();
						}
						ImGui::End();
					}
					if (!open)
						m_window->EditorWindowContext()->RemoveRenderJob(this);
				}
				virtual void CollectDependencies(Callback<Job*> addDependency) final override {}
			};

			// Editor context
			const Reference<EditorContext> m_context;

			// Editor window name
			std::string m_name;

			// True, if is open
			std::atomic<bool> m_open = true;
		};
	}

	// Expose parent class
	template<> inline void TypeIdDetails::GetParentTypesOf<Editor::EditorWindow>(const Callback<TypeId>& report) { report(TypeId::Of<Object>()); }
}

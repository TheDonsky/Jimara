#include "EditorWindow.h"


namespace Jimara {
	namespace Editor {
		void EditorWindow::Close() { m_open = false; }

		/// <summary> Base window serializer </summary>
		const Serialization::SerializerList::From<EditorWindow>* EditorWindow::Serializer() {
			class BaseEditorWindowSerializer : public virtual Serialization::SerializerList::From<EditorWindow> {
			public:
				inline BaseEditorWindowSerializer() : Serialization::ItemSerializer("EditorWindow", "EditorWindow Serializer") {}
				virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, EditorWindow* target)const final override {
					if (target == nullptr) return;
					{
						static const Reference<const GUID::Serializer> serializer = Object::Instantiate<GUID::Serializer>("GUID", "Window GUID");
						recordElement(serializer->Serialize(target->m_guid));
					}
					{
						static const Reference<const Serialization::ItemSerializer::Of<EditorWindow>> serializer =
							Serialization::StringViewSerializer::For<EditorWindow>("EditorWindowName", "Editor window name/title",
								[](EditorWindow* window) -> std::string_view { return window->EditorWindowName(); },
								[](const std::string_view& value, EditorWindow* window) { window->EditorWindowName() = value; });
						recordElement(serializer->Serialize(target));
					}
				}
			};
			static const BaseEditorWindowSerializer serializer;
			return &serializer;
		}

	
		EditorWindow::EditorWindow(EditorContext* context, const std::string_view& name, ImGuiWindowFlags flags)
			: m_context(context), m_windowFlags(flags), m_name(name) {
			class WindowDisplayJob : public JobSystem::Job {
			private:
				const Reference<EditorWindow> m_window;
			public:
				inline WindowDisplayJob(EditorWindow* window) : m_window(window) {
					m_window->EditorWindowContext()->AddRenderJob(this);
					m_window->EditorWindowContext()->AddStorageObject(m_window);
				}
				inline virtual ~WindowDisplayJob() {
					m_window->EditorWindowContext()->RemoveStorageObject(m_window);
				}
				virtual void Execute() final override {
					bool open = m_window->m_open.load();
					if (open) m_window->CreateEditorWindow();
					else {
						m_window->EditorWindowContext()->RemoveRenderJob(this);
						m_window->EditorWindowContext()->RemoveStorageObject(m_window);
					}
				}
				virtual void CollectDependencies(Callback<Job*> addDependency) final override {}
			};
			Object::Instantiate<WindowDisplayJob>(this);
		}

		inline void EditorWindow::CreateEditorWindow() {
			bool open = m_open.load();
			if (!open) return;
			std::string name = [&]() -> std::string {
				std::stringstream stream;
				stream << EditorWindowName() << "###EditorWindow_" << m_guid;
				return stream.str();
			}();
			const bool windowDrawn = ImGui::Begin(name.c_str(), &open, m_windowFlags);
			if (windowDrawn) {
				ImGui::SetWindowSize(ImVec2(384.0f, 0.0f), ImGuiCond_FirstUseEver);
				DrawEditorWindow();
			}
			ImGui::End();
			if (!windowDrawn)
				OnEditorWindowDrawSkipped();
			if (!open) 
				Close();
		}
	}
}

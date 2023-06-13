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

			/// <summary> Base window serializer </summary>
			inline static const Serialization::SerializerList::From<EditorWindow>* Serializer() {
				static const BaseEditorWindowSerializer serializer;
				return &serializer;
			}

		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Editor context </param>
			/// <param name="name"> Editor window name </param>
			/// <param name="flags"> Window flags </param>
			inline EditorWindow(EditorContext* context, const std::string_view& name, ImGuiWindowFlags flags = 0)
				: m_context(context), m_windowFlags(flags), m_name(name) {
				Object::Instantiate<WindowDisplayJob>(this);
			}

			/// <summary>
			/// Should draw the contents of the editor window
			/// </summary>
			inline virtual void DrawEditorWindow() {}

			/// <summary> Invoked, whenever the editor window is hidden and DrawEditorWindow() is not invoked </summary>
			inline virtual void OnEditorWindowDrawSkipped() {}

			/// <summary>
			/// Draws editor window
			/// <para/> Notes: 
			///		<para/> 0. By default, all this one does is that it creates an ImGui window and invokes DrawEditorWindow() to draw contents; 
			///		One can override said behaviour if the window needs something custom done;
			///		<para/> 1. Custom implementations have to call Close() manually to close the window (obviously).
			/// </summary>
			inline virtual void CreateEditorWindow() {
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
				if (!open) Close();
			}

		private:
			// Job, that invokes window runtime callbacks
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

			// Editor context
			const Reference<EditorContext> m_context;

			// Window flags
			std::atomic<ImGuiWindowFlags> m_windowFlags = 0;

			// GUID
			GUID m_guid = GUID::Generate();

			// Editor window name
			std::string m_name;

			// True, if is open
			std::atomic<bool> m_open = true;

			// Serializer
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
		};
	}

	// Expose parent class
	template<> inline void TypeIdDetails::GetParentTypesOf<Editor::EditorWindow>(const Callback<TypeId>& report) { report(TypeId::Of<Object>()); }
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Editor::EditorWindow>(const Callback<const Object*>& report) { report(Editor::EditorWindow::Serializer()); }
}

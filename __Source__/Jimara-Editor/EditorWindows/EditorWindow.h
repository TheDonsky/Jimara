#pragma once
#include "../Environment/JimaraEditor.h"
#include "../GUI/ImGuiRenderer.h"

namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Basic template for an editor window
		/// </summary>
		class JIMARA_EDITOR_API EditorWindow : public virtual Object {
		public:
			/// <summary> Editor context </summary>
			inline EditorContext* EditorWindowContext()const { return m_context; }

			/// <summary> Window name/header </summary>
			inline std::string& EditorWindowName() { return m_name; }

			/// <summary> Window name/header </summary>
			inline const std::string& EditorWindowName()const { return m_name; }

			/// <summary> Closes the window </summary>
			void Close();

			/// <summary> Base window serializer </summary>
			static const Serialization::SerializerList::From<EditorWindow>* Serializer();

		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Editor context </param>
			/// <param name="name"> Editor window name </param>
			/// <param name="flags"> Window flags </param>
			EditorWindow(EditorContext* context, const std::string_view& name, ImGuiWindowFlags flags = 0);

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
			virtual void CreateEditorWindow();

		private:
			// Editor context
			const Reference<EditorContext> m_context;

			// Window flags
			std::atomic<ImGuiWindowFlags> m_windowFlags = 0;

			// GUID
			GUID m_guid = GUID::Generate();

			// Editor window name
			std::string m_name;

			// True, if is open
			std::atomic_bool m_open = true;
		};
	}

	// Expose parent class
	template<> inline void TypeIdDetails::GetParentTypesOf<Editor::EditorWindow>(const Callback<TypeId>& report) { 
		report(TypeId::Of<Object>()); 
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Editor::EditorWindow>(const Callback<const Object*>& report) { 
		report(Editor::EditorWindow::Serializer()); 
	}
}

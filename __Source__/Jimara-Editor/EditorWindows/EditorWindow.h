#pragma once
#include "../Environment/JimaraEditor.h"
#include "../GUI/ImGuiRenderer.h"

namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Basic template for an editor window
		/// </summary>
		class EditorWindow : public virtual JobSystem::Job {
		public:
			/// <summary> Editor context </summary>
			inline EditorContext* EditorWindowContext()const { return m_context; }

			/// <summary> Window name/header </summary>
			std::string& EditorWindowName() { return m_name; }

			/// <summary> Window name/header </summary>
			const std::string& EditorWindowName()const { return m_name; }

		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Editor context </param>
			/// <param name="name"> Editor window name </param>
			inline EditorWindow(EditorContext* context, const std::string_view& name) : m_context(context), m_name(name) {}

			/// <summary>
			/// Should draw the contents of the editor window
			/// </summary>
			virtual void DrawEditorWindow() = 0;

			/// <summary> Invoked by job system to execute the task at hand (called, once all dependencies are resolved) </summary>
			virtual void Execute() final override {
				bool open = true;
				if (ImGui::Begin(EditorWindowName().c_str(), &open))
					DrawEditorWindow();
				ImGui::End();
				if (!open)
					EditorWindowContext()->RemoveRenderJob(this);
			}

			/// <summary>
			/// Should report the jobs, this particular task depends on (invoked by job system to build dependency graph)
			/// </summary>
			/// <param name="addDependency"> Calling this will record dependency for given job (individual dependencies do not have to be added to the system to be invoked) </param>
			virtual void CollectDependencies(Callback<Job*> addDependency) override {}


		private:
			// Editor context
			const Reference<EditorContext> m_context;

			// Editor window name
			std::string m_name;
		};
	}

	// Expose parent class
	template<> inline void TypeIdDetails::GetParentTypesOf<Editor::EditorWindow>(const Callback<TypeId>& report) { report(TypeId::Of<JobSystem::Job>()); }
}

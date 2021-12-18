#pragma once
namespace Jimara {
	namespace Editor {
		class ImGuiWindowContext;
	}
}
#include "../ImGuiAPIContext.h"
#include <OS/Window/Window.h>


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// ImGui context per Graphics API type and a Window
		/// </summary>
		class ImGuiWindowContext : public virtual Object {
		public:
			/// <summary> reference to the ImGui API context singleton </summary>
			inline ImGuiAPIContext* ImGuiContext()const { return m_apiContext; }

			/// <summary> Window, this context is tied to </summary>
			inline OS::Window* Window()const { return m_window; }

			/// <summary> Prepares ImGui to begin rendering to the window (invoked by ImGuiRenderer) </summary>
			virtual void BeginFrame() = 0;

			/// <summary> Makes sure, windowing-specific updates are applied when rendering is done (invoked by ImGuiRenderer) </summary>
			virtual void EndFrame() = 0;

		protected:
			/// <summary>
			/// Only the children may invoke the constructor
			/// </summary>
			/// <param name="window"> Window, this context is tied to </param>
			ImGuiWindowContext(OS::Window* window);

		private:
			// ImGui API context
			const Reference<ImGuiAPIContext> m_apiContext;

			// Window, this context is tied to
			const Reference<OS::Window> m_window;
		};
	}
}

#pragma once
namespace Jimara {
	namespace Editor {
		class ImGuiWindowContext;
	}
}
#include "ImGuiAPIContext.h"
#include <OS/Window/Window.h>


namespace Jimara {
	namespace Editor {
		class ImGuiWindowContext : public virtual ObjectCache<Reference<OS::Window>>::StoredObject {
		public:
			inline ImGuiAPIContext* ImGuiContext()const { return m_apiContext; }

			inline OS::Window* Window()const { return m_window; }

			virtual void BeginFrame() = 0;

		protected:
			ImGuiWindowContext(OS::Window* window);

		private:
			const Reference<ImGuiAPIContext> m_apiContext;
			const Reference<OS::Window> m_window;
		};
	}
}

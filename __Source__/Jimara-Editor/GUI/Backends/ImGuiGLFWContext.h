#pragma once
#include "../ImGuiWindowContext.h"
#include <OS/Window/GLFW/GLFW_Window.h>


namespace Jimara {
	namespace Editor {
		class ImGuiGLFWContext : public virtual ImGuiWindowContext {
		protected:
			ImGuiGLFWContext(OS::GLFW_Window* window);

		public:
			~ImGuiGLFWContext();

			virtual void BeginFrame() override;

		private:
			Size2 m_lastSize = Size2(0);
		};

		class ImGuiGLFWVulkanContext : public virtual ImGuiGLFWContext {
		public:
			ImGuiGLFWVulkanContext(OS::GLFW_Window* window);
		};
	}
}

#pragma once
#include "ImGuiWindowContext.h"
#include <OS/Window/GLFW/GLFW_Window.h>


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Superclass for all GLFW contexts
		/// </summary>
		class ImGuiGLFWContext : public virtual ImGuiWindowContext {
		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="apiContext"> ImGui API Context </param>
			/// <param name="window"> Window, this context is tied to </param>
			ImGuiGLFWContext(ImGuiAPIContext* apiContext, OS::GLFW_Window* window);

		public:
			/// <summary> Virtual Destructor </summary>
			~ImGuiGLFWContext();

#ifdef JIMARA_EDITOR_ImGuiRenderer_RenderFrameAtomic
			/// <summary>
			/// Renders frame in 'safe context'
			/// </summary>
			/// <param name="render"> Render callback </param>
			virtual void RenderFrame(Callback<> render) override;
#else
			/// <summary> Prepares ImGui to begin rendering to the window (invoked by ImGuiRenderer) </summary>
			virtual void BeginFrame() override;

			/// <summary> Makes sure, windowing-specific updates are applied when rendering is done (invoked by ImGuiRenderer) </summary>
			virtual void EndFrame() override;
#endif

		private:
			// Size of the window from the last frame
			Size2 m_lastSize = Size2(0);
		};

		/// <summary>
		/// ImGuiGLFWContext for Vulkan graphics
		/// </summary>
		class ImGuiGLFWVulkanContext : public virtual ImGuiGLFWContext {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="apiContext"> ImGui API Context </param>
			/// <param name="window"> Target GLFW window </param>
			ImGuiGLFWVulkanContext(ImGuiAPIContext* apiContext, OS::GLFW_Window* window);
		};
	}
}

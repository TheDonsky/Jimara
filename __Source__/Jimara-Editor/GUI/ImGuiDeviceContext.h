#pragma once
namespace Jimara {
	namespace Editor {
		class ImGuiDeviceContext;
	}
}
#include "ImGuiWindowContext.h"
#include "ImGuiRenderer.h"
#include <Graphics/GraphicsDevice.h>


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// ImGui context per graphics device
		/// </summary>
		class ImGuiDeviceContext : public virtual ObjectCache<Reference<Graphics::GraphicsDevice>>::StoredObject {
		public:
			/// <summary> reference to the ImGui API context singleton </summary>
			inline ImGuiAPIContext* APIContext()const { return m_apiContext; }

			/// <summary> Graphics device this context is tied to </summary>
			inline Graphics::GraphicsDevice* GraphicsDevice()const { return m_device; }

			/// <summary>
			/// Gets ImGuiWindowContext instance for given window
			/// Note: There will only be a single instance of an ImGuiWindowContext per window, this function will not create multiple ones.
			/// </summary>
			/// <param name="window"> Window to render to </param>
			/// <returns> Per-Window ImGui content </returns>
			virtual Reference<ImGuiWindowContext> GetWindowContext(OS::Window* window) = 0;

			/// <summary>
			/// Creates a new instance of an ImGuiRenderer tied to the given ImGuiWindowContext and the RenderEngineInfo(from some generic render engine)
			/// Notes: 
			///		0. Render engine should likely be from a surface from the same window and the same Graphics device for predictable behaviour;
			///		1. One would expect to create ImGuiRenderer objects as a part of a per-RenderEngine data for an ImageRenderer.
			/// </summary>
			/// <param name="windowContext"> Window Context </param>
			/// <param name="renderEngineInfo"> Surface render engine info </param>
			/// <returns> New instance of an ImGui renderer </returns>
			virtual Reference<ImGuiRenderer> CreateRenderer(ImGuiWindowContext* windowContext, const Graphics::RenderEngineInfo* renderEngineInfo) = 0;

		protected:
			/// <summary>
			/// Only the children may invoke the constructor
			/// </summary>
			/// <param name="device"> Graphics device this context is tied to </param>
			ImGuiDeviceContext(Graphics::GraphicsDevice* device);

		private:
			// ImGui API context
			const Reference<ImGuiAPIContext> m_apiContext;

			// Graphics device
			const Reference<Graphics::GraphicsDevice> m_device;
		};
	}
}

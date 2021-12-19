#pragma once
namespace Jimara {
	namespace Editor {
		class ImGuiDeviceContext;
	}
}
#include "ImGuiRenderer.h"
#include <Graphics/GraphicsDevice.h>


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// ImGui context per graphics device
		/// </summary>
		class ImGuiDeviceContext : public virtual Object {
		public:
			/// <summary> reference to the ImGui API context singleton </summary>
			inline ImGuiAPIContext* APIContext()const { return m_apiContext; }

			/// <summary> Graphics device this context is tied to </summary>
			inline Graphics::GraphicsDevice* GraphicsDevice()const { return m_device; }

			/// <summary>
			/// Creates a renderer
			/// </summary>
			/// <param name="renderEngineInfo"> Render engine info </param>
			/// <returns> Instance of an ImGuiRenderer </returns>
			virtual Reference<ImGuiRenderer> CreateRenderer(const Graphics::RenderEngineInfo* renderEngineInfo) = 0;

		protected:
			/// <summary>
			/// Only the children may invoke the constructor
			/// </summary>
			/// <param name="apiContext"> ImGuiApplicationContext </param>
			/// <param name="device"> Graphics device this context is tied to </param>
			inline ImGuiDeviceContext(ImGuiAPIContext* apiContext, Graphics::GraphicsDevice* device)
				: m_apiContext([&]() -> Reference<ImGuiAPIContext> {
				assert(device != nullptr);
				return (apiContext != nullptr) ? Reference<ImGuiAPIContext>(apiContext) : Object::Instantiate<ImGuiAPIContext>(device->Log()); }())
				, m_device(device) {}

		private:
			// ImGui API context
			const Reference<ImGuiAPIContext> m_apiContext;

			// Graphics device
			const Reference<Graphics::GraphicsDevice> m_device;
		};
	}
}

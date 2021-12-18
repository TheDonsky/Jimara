#pragma once
namespace Jimara {
	namespace Editor {
		class ImGuiDeviceContext;
	}
}
#include "../ImGuiAPIContext.h"
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

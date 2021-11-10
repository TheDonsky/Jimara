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
		class ImGuiDeviceContext : public virtual ObjectCache<Reference<Graphics::GraphicsDevice>>::StoredObject {
		public:
			inline ImGuiAPIContext* APIContext()const { return m_apiContext; }

			inline Graphics::GraphicsDevice* GraphicsDevice()const { return m_device; }

			virtual Reference<ImGuiWindowContext> GetWindowContext(OS::Window* window) = 0;

			virtual Reference<ImGuiRenderer> CreateRenderer(ImGuiWindowContext* windowContext, const Graphics::RenderEngineInfo* renderEngineInfo) = 0;

		protected:
			ImGuiDeviceContext(Graphics::GraphicsDevice* device);

		private:
			const Reference<ImGuiAPIContext> m_apiContext;
			const Reference<Graphics::GraphicsDevice> m_device;
		};
	}
}

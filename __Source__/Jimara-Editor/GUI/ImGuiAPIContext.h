#pragma once
#include <Graphics/GraphicsDevice.h>
#include <imgui.h>
namespace Jimara {
	namespace Editor {
		class ImGuiAPIContext;
	}
}
#include "ImGuiDeviceContext.h"


namespace Jimara {
	namespace Editor {
		class ImGuiAPIContext : public virtual Object {
		private:
			ImGuiAPIContext();

			virtual ~ImGuiAPIContext();

		public:
			static Reference<ImGuiAPIContext> GetInstance();

			Reference<ImGuiDeviceContext> GetDeviceContext(Graphics::GraphicsDevice* device);

			static std::mutex& APILock();

		protected:
			virtual void OnOutOfScope()const override;
		};
	}
}

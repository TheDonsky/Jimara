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
		/// <summary>
		/// ImGui API context/instance (only accessible as a singleton; not tied to any particular Window or Graphics Device)
		/// </summary>
		class ImGuiAPIContext : public virtual Object {
		private:
			// Constructor is private:
			ImGuiAPIContext();

			// Nobody can delete this by hand, I guess:
			virtual ~ImGuiAPIContext();

		public:
			/// <summary> Singleton instance of the API context </summary>
			static Reference<ImGuiAPIContext> GetInstance();

			/// <summary>
			/// Gets ImGuiDeviceContext instance for given graphics device
			/// Note: There will only be a single instance of an ImGuiDeviceContext per device, this function will not create multiple ones.
			/// </summary>
			/// <param name="device"> Graphics device used for rendering </param>
			/// <returns> Per-Device ImGui context </returns>
			Reference<ImGuiDeviceContext> GetDeviceContext(Graphics::GraphicsDevice* device);

			/// <summary> 
			/// ImGui operations are not thread-safe by design, so this lock is supposed to protect the high level UI renderers,
			/// such as ImGuiRenderer 
			/// </summary>
			static std::mutex& APILock();

		protected:
			/// <summary> Invoked, when ImGuiAPIContext handle is no longer held </summary>
			virtual void OnOutOfScope()const override;
		};
	}
}

#pragma once
#include <Graphics/GraphicsDevice.h>
#include <imgui.h>
namespace Jimara {
	namespace Editor {
		class ImGuiAPIContext;
	}
}
#include "ImGuiRenderer.h"


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
			/// Creates a renderer for given graphics device, targetting given window
			/// </summary>
			/// <param name="device"> Graphics device used for rendering </param>
			/// <param name="window"> Window to render on </param>
			/// <param name="renderEngineInfo"> Render engine info </param>
			/// <returns> Instance of an ImGuiRenderer </returns>
			Reference<ImGuiRenderer> CreateRenderer(Graphics::GraphicsDevice* device, OS::Window* window, const Graphics::RenderEngineInfo* renderEngineInfo);

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

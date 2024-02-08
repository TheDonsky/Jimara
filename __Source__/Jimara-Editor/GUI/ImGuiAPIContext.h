#pragma once
#include <Jimara/Graphics/GraphicsDevice.h>
#include "ImGuiIncludes.h"
#define JIMARA_EDITOR_ImGuiRenderer_RenderFrameAtomic
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
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="logger"> Logger </param>
			ImGuiAPIContext(OS::Logger* logger);

			/// <summary> Virtual destructor </summary>
			virtual ~ImGuiAPIContext();

			/// <summary>
			/// Creates a device context for given graphics device, targetting given window
			/// </summary>
			/// <param name="device"> Graphics device used for rendering </param>
			/// <param name="window"> Window to render on </param>
			/// <returns> Instance of an ImGuiRenderer </returns>
			Reference<ImGuiDeviceContext> CreateDeviceContext(Graphics::GraphicsDevice* device, OS::Window* window);

			/// <summary> Logger </summary>
			inline OS::Logger* Log()const { return m_logger; }

			/// <summary>
			/// ImGuiAPIContext lock for setting "Current active context" and preventing other threads from doing the same while this object exists
			/// </summary>
			class Lock {
			public:
				/// <summary>
				/// Constructor (Locks and sets the context)
				/// </summary>
				/// <param name="context"> ImGuiAPIContext to set </param>
				Lock(const ImGuiAPIContext* context);

				/// <summary> Destructor (removes active context and unlocks) </summary>
				virtual ~Lock();

			private:
				// Lock
				std::unique_lock<std::recursive_mutex> m_lock;

				// API Context
				Reference<const ImGuiAPIContext> m_apiContext;

				// Old context
				ImGuiContext* m_oldContext = nullptr;
				ImPlotContext* m_oldPlotContext = nullptr;
			};

		private:
			// Logger
			const Reference<OS::Logger> m_logger;

			// Context
			ImGuiContext* m_context = nullptr;
			ImPlotContext* m_imPlotContext = nullptr;
		};
	}
}

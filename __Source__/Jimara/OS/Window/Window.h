#pragma once
#include "../Logging/Logger.h"
#include "../Input/Input.h"
#include "../../Math/Math.h"
#include "../../Core/Systems/Event.h"
#include <string>
#include <shared_mutex>
#ifdef _WIN32
#include <windows.h>
#elif __APPLE__
//#include <QuartzCore/CAMetalLayer.h>
#ifndef VK_USE_PLATFORM_METAL_EXT
#define VK_USE_PLATFORM_METAL_EXT
#endif
#include <vulkan/vulkan.h>
#else
#include <X11/X.h>
#include <X11/Xlib.h>
#include <xcb/xproto.h>
#include <X11/Xlib-xcb.h> 
#endif

namespace Jimara {
	namespace OS {
		/// <summary> 
		/// Generic interface for an OS Window 
		/// </summary>
		class JIMARA_API Window : public virtual Object {
		public:
			/// <summary> Window manager backend (some library, that handles the actual low level stuff) </summary>
			enum class JIMARA_API Backend : uint8_t {
				/// <summary> GLFW backend </summary>
				GLFW = 0,

				/// <summary> Not an actual backend; tells how many different backend types are available </summary>
				BACKEND_OPTION_COUNT = 1
			};

			/// <summary>
			/// Creates a new window
			/// </summary>
			/// <param name="logger"> Logger </param>
			/// <param name="name"> Window name/title (will be displayed on top) </param>
			/// <param name="size"> Window size hint (actual window may end up with slightly different dimensions, if the windowing system or the monitor does not support the one provided) </param>
			/// <param name="resizable"> If true, the user is supposed to be able to resize the window </param>
			/// <param name="backend"> Window manager backend </param>
			/// <returns> New instance of the window </returns>
			static Reference<Window> Create(Logger* logger, const std::string& name = "Jimara", Size2 size = Size2(1280, 720), bool resizable = true, Backend backend = Backend::GLFW);

			/// <summary> Virtual destructor </summary>
			virtual ~Window();

			/// <summary> Window logger </summary>
			Logger* Log()const;

			/// <summary> Window title </summary>
			virtual std::string Name()const = 0;

			/// <summary> Changes window title </summary>
			virtual void SetName(const std::string& newName) = 0;

			/// <summary> True, if the window is in full-screen mode </summary>
			virtual bool IsFullscreen()const = 0;

			/// <summary> Switches between windowed and fullscreen modes </summary>
			virtual void SetFullscreen(bool fullscreen) = 0;

			/// <summary> True, when the user closes the window </summary>
			virtual bool Closed()const = 0;

			/// <summary> Halts thread till window is closed (keep in mind that this is completely unsafe to invoke from the said window's update thread) </summary>
			virtual void WaitTillClosed() = 0;

			/// <summary> Current frame buffer dimensions </summary>
			virtual Size2 FrameBufferSize()const = 0;

			/// <summary> Event invoked on update (every time the api handles window events) </summary>
			virtual Event<Window*>& OnUpdate() = 0;

			/// <summary> Invoked when the window dimensions change </summary>
			virtual Event<Window*>& OnSizeChanged() = 0;

			/// <summary> Locks event handling </summary>
			virtual std::shared_mutex& MessageLock() = 0;

			/// <summary>
			/// Instantiates a compatible Input module
			/// </summary>
			/// <returns> New instance of an Input module </returns>
			virtual Reference<Input> CreateInputModule() = 0;

#ifdef _WIN32
			/// <summary> Underlying Win32 window handle </summary>
			virtual HWND GetHWND() = 0;

#else //__APPLE__
			/// <summary>
			///	Creates Vulkan surface (used by Vulkan backend)
			/// </summary>
			/// <param name="vkInstancePtr"> VkInstance* </param>
			/// <param name="vkSurfaceKHRPtr"> VkSurfaceKHR* (for results) </param>
			virtual void MakeVulkanSurface(void* vkInstancePtr, void* vkSurfaceKHRPtr) = 0;
#endif
#ifdef EXPOSE_LINUX_WINDOW_DETAILS
			/// <summary>
			/// Window manager on linux
			/// </summary>
			enum class WindowManager {
				/// <summary> X11 </summary>
				X11,

				/// <summary> Wayland </summary>
				WAYLAND
			};

			/// <summary> Window manager backend for this window </summary>
			virtual WindowManager GetWindowManager()const = 0;

			/// <summary> Underlying xcb_connection_t for X11 window (linux) </summary>
			virtual xcb_connection_t* GetConnectionXCB() = 0;

			/// <summary> Underlying xcb_window_t for X11 window (linux) </summary>
			virtual xcb_window_t GetWindowXCB() = 0;

			/// <summary> Underlying wl_display* for WAYLAND window (linux) </summary>
			virtual void* GetWaylandDisplay() = 0;

			/// <summary> Underlying wl_surface* for WAYLAND window (linux) </summary>
			virtual void* GetWaylandSurface() = 0;
#endif

		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="logger"> Logger </param>
			Window(Logger* logger);

		private:
			// Logger
			Reference<Logger> m_logger;
		};
	}
}
